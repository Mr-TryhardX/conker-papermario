#!/usr/bin/env python3
"""Exercise shared vertex slots, bone transforms and real reference integrity."""
import json,math,struct,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from extract_guest_actor import Guest,load_vertices,display_list

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
