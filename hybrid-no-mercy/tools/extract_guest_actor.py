#!/usr/bin/env python3
"""Reconstruct a posed actor from a local, big-endian No Mercy guest snapshot.

This reads renderer outputs, not host memory. It does not infer animation codecs.
The snapshot and generated original-game assets must remain local.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path
import numpy as np
from PIL import Image

class Guest:
    def __init__(self, data):
        if len(data) != 0x800000: raise ValueError('Expected 8 MiB logical guest RDRAM')
        self.data = data
    def read(self, address, size):
        address &= 0xffffff
        if address + size > len(self.data): raise ValueError('Guest address out of bounds')
        return self.data[address:address+size]
    def unpack(self, fmt, address):
        return struct.unpack(fmt, self.read(address, struct.calcsize(fmt)))
    def word(self, address): return self.unpack('>I', address)[0]
    def matrix(self, address):
        whole = self.unpack('>16h', address)
        fraction = self.unpack('>16H', address+32)
        return np.array([a+b/65536 for a,b in zip(whole,fraction)]).reshape(4,4)

def display_list(g, address):
    """Bounded F3DEX2 draw-list traversal; fail on unsupported geometry commands."""
    commands=[]; stack=[address]; budget=4096
    while stack:
        pc=stack.pop()
        while budget:
            budget-=1; a,b=g.unpack('>II',pc);pc+=8;op=a>>24
            if op==0xdf: break
            if op==0xde:
                if (a>>16)&1: pc=b&0xffffff
                else: stack.append(pc);pc=b&0xffffff
                continue
            commands.append((a,b))
        else: raise ValueError('Display list budget exceeded')
    return commands

def load_vertices(g,a,b,current,inverse,cache):
    n=(a>>12)&255;end=(a>>1)&127;start=end-n
    if current is None or start<0 or not 0<n<=64:raise ValueError('Invalid vertex load')
    for i in range(n):
        x,y,z,flag,u,v,*normal=g.unpack('>hhhHhh4B',b+i*16)
        q=np.array([x,y,z,1])@current@inverse
        if abs(q[3])<1e-7:raise ValueError('Invalid homogeneous vertex')
        cache[start+i]=list(q[:3]/q[3])+[u/32,v/32]

def parse_part(g, obj, out):
    bone=g.word(obj+4)&0xffffff
    vertices=g.word(obj+0x30)&0xffffff
    if not vertices:return None
    commands=display_list(g,g.word(obj+0x34)&0xffffff)
    faces=[];palette=None; pixels=None;bits=None;w=h=None;mirror=[False,False]
    for a,b in commands:
        op=a>>24
        if op in (5,6):
            words=[a] if op==5 else [a,b]
            faces.extend([[(v>>s)&255 for s in (16,8,0)] for v in words])
        elif op==0xfd:
            if palette is None: palette=b&0xffffff
            else: pixels=b&0xffffff
        elif op==0xf5 and ((b>>24)&7)==0:
            if ((a>>21)&7)!=2: raise ValueError('Expected CI material')
            bits={0:4,1:8}.get((a>>19)&3)
            mirror=[bool((b>>8)&1),bool((b>>18)&1)]
        elif op==0xf2 and ((b>>24)&7)==0:
            w=(((b>>12)&4095)-((a>>12)&4095))//4+1
            h=((b&4095)-(a&4095))//4+1
        elif op==1: raise ValueError('Explicit vertex loads need separate cache decoding')
    if not faces:return None
    if any(v&1 for f in faces for v in f):raise ValueError('Odd F3DEX2 vertex index')
    faces=[[v//2 for v in f] for f in faces]
    count=max(max(f) for f in faces)+1
    if not vertices or count>128:raise ValueError('Invalid implicit vertex buffer')
    if not bits or not w or not h or not palette or not pixels or w*h>65536:
        raise ValueError('Incomplete or oversized CI material')
    colors=g.unpack('>'+str(1<<bits)+'H',palette)
    rgba=[tuple(((c>>s)&31)*255//31 for s in (11,6,1))+(255 if c&1 else 0,) for c in colors]
    packed=g.read(pixels,(w*h*bits+7)//8)
    indices=list(packed) if bits==8 else [v for b in packed for v in (b>>4,b&15)]
    name=f'{bone:06X}.png';im=Image.new('RGBA',(w,h));im.putdata([rgba[i] for i in indices[:w*h]]);im.save(out/name)
    return {'bone':bone,'object':obj,'texture':name,'vertices':[], 'faces':faces,
            'display_list':g.word(obj+0x34)&0xffffff,'texture_size':[w,h],'mirror':mirror,
            'clip_matrix':g.matrix(obj+0x38).tolist(),
            'reference_matrix':list(g.unpack('>16f',bone+0x1c))}

def run(snapshot,out):
    data=snapshot.read_bytes();g=Guest(data);out.mkdir(parents=True,exist_ok=True)
    parts=[]
    for i in range(128):
        obj=g.word(0x61d40+i*4)&0xffffff
        if not 0<obj<0x800000-0xb8 or g.word(obj)!=1:continue
        bone=g.word(obj+4)&0xffffff
        # Reference actor's resident skeleton; exclude UI and other actors.
        if not 0x91cc0<=bone<=0x93160:continue
        p=parse_part(g,obj,out)
        if p:parts.append(p)
    if len(parts)<20:raise ValueError('Incomplete reference actor')
    first=next(p for p in parts if abs(np.linalg.det(np.array(p['reference_matrix']).reshape(4,4)))>.1)
    camera=np.linalg.inv(np.array(first['reference_matrix']).reshape(4,4))@np.array(first['clip_matrix'])
    inverse=np.linalg.inv(camera)
    # The caller fills a shared vertex cache in several chunks, sometimes using
    # different bone matrices. The object's pointer is not vertex-cache slot 0.
    targets={p['display_list']:p for p in parts};calls=[]
    for target in targets:
        needle=struct.pack('>II',0xde000000,target);at=data.find(needle);matches=[]
        while at>=0:
            if at%8==0 and 0x380000<=at<0x400000:matches.append(at)
            at=data.find(needle,at+1)
        if matches:calls.append(max(matches))
    if len(calls)!=len(parts):raise ValueError('Could not locate unique actor draw calls')
    cache={};current=None;seen=set()
    for pc in range(min(calls)-512,max(calls)+8,8):
        a,b=g.unpack('>II',pc);op=a>>24
        if op==0xdc and (a&255)==0x0e:current=g.matrix(b)
        elif op==1 and current is not None:
            # The bounded preamble can begin inside another actor's draw.
            # Ignore its loads until a force-matrix establishes this cache.
            load_vertices(g,a,b,current,inverse,cache)
        elif op==0xde and (b&0xffffff) in targets:
            p=targets[b&0xffffff];count=max(max(f) for f in p['faces'])+1
            if any(i not in cache for i in range(count)):raise ValueError('Incomplete vertex cache')
            w,h=p['texture_size'];p['vertices']=[cache[i][:3]+[cache[i][3]/w,cache[i][4]/h] for i in range(count)]
            p['matrix']=np.eye(4).tolist();seen.add(p['bone'])
    if len(seen)!=len(parts):raise ValueError('Missing draw parts')
    for p in parts:
        p.pop('clip_matrix')
        p.pop('reference_matrix')
        if not all(math.isfinite(v) for row in p['matrix'] for v in row):raise ValueError('Nonfinite actor transform')
    record={'source':'No Mercy original renderer reference pose; no skeletal codec inferred',
            'snapshot_sha256':hashlib.sha256(data).hexdigest(),'parts':parts}
    (out/'actor.json').write_text(json.dumps(record)+'\n')
    print(f'Reconstructed {len(parts)} textured draw parts, {sum(len(p["faces"]) for p in parts)} triangles')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('snapshot',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();run(a.snapshot,a.output)
