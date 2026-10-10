#!/usr/bin/env python3
"""Verify measured pair transforms and reject unsynchronized captures."""
import copy,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from compose_captured_pair import compose
camera=np.array([[2,0,0,0],[0,3,0,0],[0,0,1,1],[0,0,-4,0]],float)
relative=np.eye(4);relative[3,:3]=[10,0,5]
a={'snapshot_sha256':'same','actor_index':0,'actor_to_clip_matrix':camera.tolist(),
   'parts':[{'vertices':[[1,2,3,.2,.3]],'matrix':np.eye(4).tolist(),'texture':'skin.png'}]}
b=copy.deepcopy(a);b['actor_index']=1;b['actor_to_clip_matrix']=(relative@camera).tolist()
p=compose(a,b)
assert np.allclose(p['relative_actor_matrix'],relative)
assert np.allclose(p['parts'][1]['vertices'],[[11,2,8,.2,.3]])
assert p['parts'][1]['texture']=='actor-1-skin.png'
assert a['parts'][0]['vertices']==[[1,2,3,.2,.3]]
for change in ({'snapshot_sha256':'different'},{'actor_index':0},{'actor_to_clip_matrix':[[float('nan')]*4]*4}):
 bad=copy.deepcopy(b);bad.update(change)
 try:compose(a,bad)
 except ValueError:pass
 else:raise AssertionError('Invalid paired capture accepted')
bad=copy.deepcopy(a);bad['actor_to_clip_matrix']=np.zeros((4,4)).tolist()
try:compose(bad,b)
except ValueError:pass
else:raise AssertionError('Singular coordinate space accepted')
print('PASS: shared projection cancellation, measured offset, UV preservation, source isolation and malformed pairs.')
