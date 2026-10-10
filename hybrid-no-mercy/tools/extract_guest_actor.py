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

def actor_graphics_task(g):
    # No Mercy submits separate F3DEX2 actor and S2DEX UI tasks. The
    # scheduler's scratch task at 0x8e6f0 can be the later UI submission.
    # Locate the bounded resident F3DEX2 task, never a call beyond its size.
    tasks=[]
    marker=struct.pack('>II',1,2)
    offset=0x380000
    while True:
        task=g.data.find(marker,offset,0x400000-56)
        if task<0:break
        offset=task+8
        if task%8:continue
        if g.word(task+8)&0xffffff!=0x40380:continue
        if g.word(task+0x10)&0xffffff!=0x42450:continue
        root=g.word(task+0x30)&0xffffff;size=g.word(task+0x34)
        if 0x380000<=root<0x400000 and 8<=size<=0x20000 and size%8==0:
            tasks.append((task,root,size))
    if len(tasks)!=1:raise ValueError('Missing or ambiguous actor graphics task')
    return tasks[0]

def actor_base(actor_index):
    if actor_index not in range(4): raise ValueError("Actor index must be 0..3")
    return 0x91cc0 + actor_index * 0x1810

def replay_actor_task(g,parts,inverse,actor_index=0):
    base=actor_base(actor_index); last=base+24*0xdc
    task,root,size=actor_graphics_task(g)
    targets={p['display_list']:p for p in parts}
    cache={};bindings={};matrix_targets={};local_vertices={};current=None;current_bone=None;current_address=None;seen=set();segments=[0]*16
    matrix_bones={p['object']+0x38:p['bone'] for p in parts if 'object' in p}
    # Non-drawing joints can load shared vertices used by another draw part.
    for bone in range(base,last+1,0xdc):
        obj=g.word(bone+0x14)&0xffffff
        if 0<obj<0x800000-0x78:matrix_bones[obj+0x38]=bone
    for i in range(256):
        obj=g.word(0x61d40+i*4)&0xffffff
        if not 0<obj<0x800000-0x78 or g.word(obj)!=1:continue
        bone=g.word(obj+4)&0xffffff
        if base<=bone<=last:matrix_bones[obj+0x38]=bone
    pc=root;limit=root+size;stack=[];budget=65536
    while budget:
        budget-=1
        if pc%8 or not 0<=pc<=0x800000-8 or (limit is not None and pc+8>limit):
            raise ValueError('Submitted command outside display-list bounds')
        a,b=g.unpack('>II',pc);pc+=8;op=a>>24
        def resolve(address):
            segment=address>>24
            if segment in (0x80,0xA0):
                result=address&0xffffff
            elif segment<16:result=segments[segment]+(address&0xffffff)
            else:raise ValueError('Invalid display-list segment')
            if not 0<=result<0x800000:raise ValueError('Resolved address outside guest RAM')
            return result
        if op==0xdb and ((a>>16)&255)==6:
            segment=(a&0xffff)//4
            if segment>=16:raise ValueError('Invalid segment assignment')
            segments[segment]=b&0xffffff
        elif op==0xdc and (a&255)==0x0e:
            address=resolve(b)
            current=g.matrix(address);current_bone=matrix_bones.get(address);current_address=address
            if current_bone is None and address>=0x38 and g.word(address-0x38)==1:
                bone=g.word(address-0x34)&0xffffff
                if base<=bone<=last and (bone-base)%0xdc==0:
                    current_bone=bone
        elif op==1:
            load_vertices(g,a,resolve(b),current,inverse,cache)
            n=(a>>12)&255;start=((a>>1)&127)-n
            for i in range(start,start+n):
                bindings[i]=current_bone;matrix_targets[i]=current_address
                local_vertices[i]=list(g.unpack('>3h',resolve(b)+(i-start)*16))
        elif op==0xde:
            target=resolve(b)
            if target in targets:
                p=targets[target];count=max(max(f) for f in p['faces'])+1
                if any(i not in cache for i in range(count)):
                    raise ValueError('Incomplete submitted vertex cache')
                w,h=p['texture_size']
                p['vertices']=[cache[i][:3]+[cache[i][3]/w,cache[i][4]/h] for i in range(count)]
                p['vertex_bones']=[bindings.get(i) for i in range(count)]
                p['vertex_matrix_targets']=[matrix_targets.get(i) for i in range(count)]
                p['vertex_local_positions']=[local_vertices[i] for i in range(count)]
                p['matrix']=np.eye(4).tolist();seen.add(p['bone'])
            if not (a>>16)&1:stack.append((pc,limit))
            pc=target;limit=None
        elif op==0xdf:
            if not stack:break
            pc,limit=stack.pop()
    else:raise ValueError('Submitted display-list budget exceeded')
    return seen,task,root,size

def run(snapshot,out,actor_index=0):
    base=actor_base(actor_index);last=base+24*0xdc
    data=snapshot.read_bytes();g=Guest(data);out.mkdir(parents=True,exist_ok=True)
    parts=[]
    # A manager-filled ring places the second wrestler at entries 132..177.
    # Bound candidate discovery to 256 words and validate every object/joint;
    # the submitted task replay below must still confirm every chosen part.
    for i in range(256):
        obj=g.word(0x61d40+i*4)&0xffffff
        if not 0<obj<0x800000-0xb8 or g.word(obj)!=1:continue
        bone=g.word(obj+4)&0xffffff
        if not base<=bone<=last or (bone-base)%0xdc:continue
        p=parse_part(g,obj,out)
        if p:parts.append(p)
    if len(parts)<20:raise ValueError('Incomplete reference actor')
    first=next(p for p in parts if abs(np.linalg.det(np.array(p['reference_matrix']).reshape(4,4)))>.1)
    camera=np.linalg.inv(np.array(first['reference_matrix']).reshape(4,4))@np.array(first['clip_matrix'])
    inverse=np.linalg.inv(camera)
    seen,task,root,size=replay_actor_task(g,parts,inverse,actor_index)
    if len(seen)!=len(parts):raise ValueError('Missing draw parts')
    for p in parts:
        p.pop('clip_matrix')
        p.pop('reference_matrix')
        if not all(math.isfinite(v) for row in p['matrix'] for v in row):raise ValueError('Nonfinite actor transform')
    record={'source':'No Mercy original renderer reference pose; no skeletal codec inferred',
            'snapshot_sha256':hashlib.sha256(data).hexdigest(),
            'graphics_task':task,'display_list_root':root,'display_list_bytes':size,
            'actor_index':actor_index,'actor_skeleton_base':base,
            'actor_to_clip_matrix':camera.tolist(),
            'actor_root_matrix':list(g.unpack('>16f',base+0x1c)),'parts':parts}
    (out/'actor.json').write_text(json.dumps(record)+'\n')
    print(f'Reconstructed {len(parts)} textured draw parts, {sum(len(p["faces"]) for p in parts)} triangles')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('snapshot',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--actor-index',type=int,choices=range(4),default=0)
    a=p.parse_args();run(a.snapshot,a.output,a.actor_index)
