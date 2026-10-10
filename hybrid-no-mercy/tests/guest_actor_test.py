#!/usr/bin/env python3
"""Exercise shared vertex slots, bone transforms and real reference integrity."""
import json,math,struct,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from extract_guest_actor import Guest,load_vertices,display_list,actor_graphics_task,replay_actor_task,actor_base

ram=bytearray(0x800000)
struct.pack_into('>hhhHhh4B',ram,256,2,3,4,0,64,96,255,255,255,255)
g=Guest(bytes(ram));cache={};identity=np.eye(4);translated=np.eye(4);translated[3,:3]=[10,20,30]
load_vertices(g,0x01001006,256,identity,identity,cache) # one vertex, cache slot 2
load_vertices(g,0x01001002,256,translated,identity,cache) # neighboring bone, slot 0
assert list(cache)==[2,0] and cache[2]==[2,3,4,2,3] and cache[0]==[12,23,34,2,3]
for call in [lambda:Guest(b'bad'),lambda:g.read(0x7fffff,2),lambda:load_vertices(g,0x01001000,256,identity,identity,{}),lambda:load_vertices(g,0x01001002,256,None,identity,{})]:
    try:call()
    except ValueError:pass
    else:raise AssertionError('Malformed renderer data accepted')
struct.pack_into('>II',ram,1024,0xde010000,1024)
try:display_list(Guest(bytes(ram)),1024)
except ValueError:pass
else:raise AssertionError('Unbounded recursive display list accepted')
# Actor and UI tasks coexist; stale DE commands after DF must not render.
struct.pack_into('>16I',ram,0x380800,1,2,0x80040380,0xD0,
    0x80042450,0x1000,0,0,0,0,0,0,0x80380000,40,0,0)
struct.pack_into('>16h16H',ram,0x5000,*[1 if i%5==0 else 0 for i in range(16)],*[0]*16)
# A matrix loaded by a non-drawing object still owns its shared vertices.
struct.pack_into('>II',ram,0x5000-0x38,1,0x80091D9C)
commands=[(0xDB060004,0x2000),(0xDC38000E,0x5000),
          (0x01001002,256),(0xDE000000,0x01000000),(0xDF000000,0),
          (0xDE000000,0x3000)]
for i,(a,b) in enumerate(commands):struct.pack_into('>II',ram,0x380000+i*8,a,b)
struct.pack_into('>II',ram,0x2000,0xDF000000,0)
parts=[{'bone':1,'display_list':0x2000,'faces':[[0,0,0]],'texture_size':[1,1]}]
seen,task,dl,size=replay_actor_task(Guest(bytes(ram)),parts,identity)
assert (seen,task,dl,size)==({1},0x380800,0x380000,40)
assert parts[0]['vertices']==[[2,3,4,2,3]]
assert parts[0]['vertex_bones']==[0x91D9C]
assert parts[0]['vertex_matrix_targets']==[0x5000]
assert parts[0]['vertex_local_positions']==[[2,3,4]]
# A second actor must bind its own joint, never the first actor's address.
struct.pack_into('>II',ram,0x5000-0x38,1,0x800935AC)
replay_actor_task(Guest(bytes(ram)),parts,identity,1)
assert parts[0]['vertex_bones']==[0x935AC]
replay_actor_task(Guest(bytes(ram)),parts,identity,0)
assert parts[0]['vertex_bones']==[None]
for invalid in (-1,4):
    try:actor_base(invalid)
    except ValueError:pass
    else:raise AssertionError('Invalid wrestler slot accepted')
# A shorter task cannot read a stale end/call beyond its submitted size.
struct.pack_into('>I',ram,0x380800+0x34,24)
try:replay_actor_task(Guest(bytes(ram)),parts,identity)
except ValueError:pass
else:raise AssertionError('Task replay escaped submitted size')
struct.pack_into('>I',ram,0x380800+0x34,40)
# UI task is distinct; two actor tasks must be rejected as ambiguous.
ram[0x380900:0x380940]=ram[0x380800:0x380840]
try:actor_graphics_task(Guest(bytes(ram)))
except ValueError:pass
else:raise AssertionError('Ambiguous actor task accepted')
print('PASS: submitted actor task, nested segment calls, stale tail exclusion and task bounds.')
root=Path(__file__).resolve().parents[1]/'asset-import/kane-animation'
if root.exists():
    frames=json.loads((root/'poses.json').read_text())['poses'];assert len(frames)==40
    for frame in frames:
        assert len(frame['parts'])==21 and sum(len(p['faces'])for p in frame['parts'])==777
        for p in frame['parts']:
            assert (root/p['texture']).is_file()
            assert all(0<=i<len(p['vertices'])for f in p['faces']for i in f)
            assert all(math.isfinite(x)for v in p['vertices']for x in v)
    assert frames[0]['parts'][0]['vertices']!=frames[20]['parts'][0]['vertices']
    print('PASS: 40 real reference poses, 21 parts each, bounded topology and actual motion.')
print('PASS: shared vertex-cache loads, independent bone transforms and malformed-data checks.')
