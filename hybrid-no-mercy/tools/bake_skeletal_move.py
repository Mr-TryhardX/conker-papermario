#!/usr/bin/env python3
"""Offline geometry experiment driven by decoded No Mercy skeletal records.

Uses the original builder in private RAM and exact captured per-vertex joint
bindings. Does not hook combat, apply damage, or position a second character.
"""
import argparse, copy, json, math, shutil, struct, subprocess
from pathlib import Path
import numpy as np
from decode_nomercy_animation import decode_frame

ROOT=Path(__file__).resolve().parents[1]

def bake(capture,reference,animation,output):
    ram=capture.read_bytes()
    if len(ram)!=0x800000:raise ValueError('Expected complete logical RAM capture')
    actor=json.loads((reference/'actor.json').read_text())
    actor_index=actor.get('actor_index',0)
    if actor_index not in range(4):raise ValueError('Invalid actor index')
    base=0x91cc0+actor_index*0x1810
    import hashlib
    if actor['snapshot_sha256']!=hashlib.sha256(ram).hexdigest():
        raise ValueError('Reference geometry and RAM capture differ')
    raw=animation.read_bytes()
    if len(raw)<102 or (len(raw)-4)%98 or (len(raw)-4)//98>120:
        raise ValueError('Invalid or oversized move record')
    old={i:np.array(struct.unpack_from('>16f',ram,base+i*220+28)).reshape(4,4) for i in range(25)}
    attachment_local={i:old[i].copy() for i in range(20,25)}
    # Attachment matrices are local to their task parent; the nineteen pose
    # matrices are already assembled in model space by the original builder.
    def attachment_parent(i):
        parent=struct.unpack_from('>I',ram,base+i*220)[0]&0xffffff
        if not base<=parent<base+25*220 or (parent-base)%220:
            raise ValueError('Invalid attachment parent')
        return (parent-base)//220
    if any(b is None for p in actor['parts'] for b in p['vertex_bones']):
        raise ValueError('Unbound source vertices')
    for p in actor['parts']:
        if len(p.get('vertex_local_positions',[]))!=len(p['vertices']) or len(p['vertex_bones'])!=len(p['vertices']):
            raise ValueError('Regenerate reference with exact local vertex positions')
        if any(not base<=b<=base+24*220 or (b-base)%220 for b in p['vertex_bones']):
            raise ValueError('Vertex bound outside selected actor skeleton')
    output.mkdir(parents=True,exist_ok=False)
    for p in actor['parts']:shutil.copy2(reference/p['texture'],output/p['texture'])
    work=ROOT/'artifacts/skeletal-move-bake';work.mkdir(parents=True,exist_ok=True)
    executable=ROOT/'artifacts/skeleton-reference/reference-skeleton'
    poses=[]
    for offset in range(4,len(raw),98):
        private=bytearray(ram)
        for joint in decode_frame(raw[offset:offset+98]):
            address=0xa1130+actor_index*2852+joint['joint']*124
            struct.pack_into('>3f',private,address,*joint['angles_degrees_xyz'])
            if joint['translation_xyz'] is not None:
                struct.pack_into('>3f',private,address+12,*joint['translation_xyz'])
        scratch=work/'private-frame.ram';scratch.write_bytes(private)
        matrices=subprocess.run([executable,scratch,str(actor_index)],stdout=subprocess.PIPE,check=True).stdout
        if len(matrices)!=1280:raise ValueError('Short matrix output')
        new={i:np.array(struct.unpack_from('<16f',matrices,i*64)).reshape(4,4) for i in range(20)}
        def transformed(i,seen=None):
            if i in new:return new[i]
            seen=set() if seen is None else seen
            if i in seen:raise ValueError('Cyclic attachment hierarchy')
            seen.add(i)
            j=attachment_parent(i)
            new[i]=attachment_local[i]@transformed(j,seen)
            return new[i]
        frame=copy.deepcopy(actor)
        frame['source']='Decoded original skeletal frame with captured vertex bindings; offline experiment'
        for p in frame['parts']:
            for vertex,bone,local in zip(p['vertices'],p['vertex_bones'],p['vertex_local_positions']):
                i=(bone-base)//220
                point=np.array(local+[1.0])@transformed(i)
                if not np.isfinite(point).all() or abs(point[3])<1e-8:
                    raise ValueError('Invalid transformed vertex')
                vertex[:3]=(point[:3]/point[3]).tolist()
        poses.append(frame)
    (output/'poses.json').write_text(json.dumps({'mode':'Offline decoded skeleton geometry experiment',
        'animation':animation.stem,'poses':poses})+'\n')
    print(f'Baked {len(poses)} decoded poses; {len(actor["parts"])} parts with per-vertex joint bindings.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture',type=Path);p.add_argument('reference',type=Path)
    p.add_argument('animation',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();bake(a.capture,a.reference,a.animation,a.output)
