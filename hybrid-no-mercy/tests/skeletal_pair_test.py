"""Authored placement direction, UV preservation and source rejection checks."""
import copy, hashlib, json, sys, tempfile
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from compose_skeletal_pair import compose

with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);sources=[]
    for actor in range(2):
        folder=root/str(actor);folder.mkdir();sources.append(folder)
        pose={'actor_index':actor,'snapshot_sha256':'shared-bind-capture',
              'joint_pivots_model_space':[[1,2,3]]*19,
              'parts':[{'matrix':np.eye(4).tolist(),'texture':'tex.bin',
                        'vertices':[[1,2,3,.25,.75],[2,2,3,.5,.75],[1,3,3,.25,.5]],
                        'faces':[[0,1,2]],'mirror':[False,False]}]}
        (folder/'tex.bin').write_bytes(b'source-texture')
        (folder/'poses.json').write_text(json.dumps({'animation':str(actor),'poses':[pose]}))
    before=[hashlib.sha256((p/'poses.json').read_bytes()).hexdigest() for p in sources]
    compose(*sources,root/'good')
    result=json.loads((root/'good/poses.json').read_text());frame=result['poses'][0]
    assert np.allclose(frame['parts'][0]['vertices'][0],[1,2,3,.25,.75])
    assert np.allclose(frame['parts'][1]['vertices'][0],[-8,2,-64,.25,.75])
    assert np.allclose(frame['paired_joint_pivots_model_space'][1][0],[-8,2,-64])
    assert result['original_root_track_verified'] is False and result['contact_verified'] is False
    assert [hashlib.sha256((p/'poses.json').read_bytes()).hexdigest() for p in sources]==before
    def reject(name,change):
        original=json.loads((sources[1]/'poses.json').read_text());bad=copy.deepcopy(original)
        change(bad);(sources[1]/'poses.json').write_text(json.dumps(bad))
        try:
            try:compose(*sources,root/name)
            except ValueError:pass
            else:raise AssertionError(name+' accepted')
            assert not (root/name).exists(), 'Invalid source created output'
        finally:(sources[1]/'poses.json').write_text(json.dumps(original))
    reject('count',lambda d:d['poses'].append(copy.deepcopy(d['poses'][0])))
    reject('same-actor',lambda d:d['poses'][0].update(actor_index=0))
    reject('bind-mismatch',lambda d:d['poses'][0].update(snapshot_sha256='different'))
    reject('bad-pivot',lambda d:d['poses'][0]['joint_pivots_model_space'][0].__setitem__(0,float('nan')))
    reject('double-transform',lambda d:d['poses'][0]['parts'][0]['matrix'][3].__setitem__(0,1))
    reject('bad-face',lambda d:d['poses'][0]['parts'][0]['faces'][0].__setitem__(0,99))
    reject('texture-escape',lambda d:d['poses'][0]['parts'][0].update(texture='../outside.bin'))
    try:compose(*sources,root/'good')
    except FileExistsError:pass
    else:raise AssertionError('Existing output overwritten')
print('Skeletal pair: placement direction, pivots, UVs, input preservation and invalid-source rejection passed.')
