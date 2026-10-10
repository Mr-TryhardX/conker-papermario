#!/usr/bin/env python3
"""Place synchronized original-renderer actors in one captured coordinate space.

This preserves measured relative transforms; it does not infer a move, contact,
or damage. Original geometry and textures remain local.
"""
import argparse, copy, json, shutil
from pathlib import Path
import numpy as np

def compose(first, second):
    if first['snapshot_sha256'] != second['snapshot_sha256']:
        raise ValueError('Actors were not captured in the same frame')
    if first['actor_index'] == second['actor_index']:
        raise ValueError('Expected distinct actors')
    cameras=[np.asarray(a['actor_to_clip_matrix'],dtype=float) for a in (first,second)]
    if any(c.shape!=(4,4) or not np.isfinite(c).all() for c in cameras):
        raise ValueError('Invalid actor-to-clip matrix')
    try:relative=cameras[1]@np.linalg.inv(cameras[0])
    except np.linalg.LinAlgError as e:raise ValueError('Singular shared coordinate transform') from e
    roots=[np.asarray(a['actor_root_matrix'],dtype=float).reshape(4,4) for a in (first,second)]
    if any(not np.isfinite(m).all() for m in roots):raise ValueError('Nonfinite actor root')
    try:expected=roots[1]@np.linalg.inv(roots[0])
    except np.linalg.LinAlgError as e:raise ValueError('Singular actor root') from e
    error=float(np.max(np.abs(relative-expected)))
    if error>.01 or any(a.get('camera_matrix_max_disagreement',float('inf'))>.01 for a in (first,second)):
        raise ValueError('Pose and renderer matrices disagree; capture a settled paused frame')
    result=copy.deepcopy(first);result['parts']=[]
    result['relative_root_max_error']=error

    for actor, transform in ((first,np.eye(4)),(second,relative)):
        for original in actor['parts']:
            part=copy.deepcopy(original)
            vertices=np.asarray(part['vertices'],dtype=float)
            if vertices.ndim!=2 or vertices.shape[1]!=5 or not np.isfinite(vertices).all():
                raise ValueError('Invalid captured vertices')
            matrix=np.asarray(part['matrix'],dtype=float)
            points=np.c_[vertices[:,:3],np.ones(len(vertices))]@matrix@transform
            if not np.isfinite(points).all() or np.any(np.abs(points[:,3])<1e-8):
                raise ValueError('Invalid paired homogeneous coordinates')
            vertices[:,:3]=points[:,:3]/points[:,3,None]
            part['vertices']=vertices.tolist();part['matrix']=np.eye(4).tolist()
            part['texture']=f"actor-{actor['actor_index']}-"+part['texture']
            result['parts'].append(part)
    result['source']='Synchronized captured pair using measured relative actor-to-clip transforms; move/contact unverified'
    result['paired_actor_indices']=[first['actor_index'],second['actor_index']]
    result['relative_actor_matrix']=relative.tolist()
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--actors',type=int,nargs=2,default=[0,1],choices=range(4))
    a=p.parse_args();sources=[a.capture/f'actor-{i}' for i in a.actors]
    records=[json.loads((s/'poses.json').read_text()) for s in sources]
    if records[0]['capture_times_seconds']!=records[1]['capture_times_seconds']:
        raise ValueError('Capture timestamps differ')
    if len(records[0]['poses'])!=len(records[1]['poses']) or not records[0]['poses']:
        raise ValueError('Empty or mismatched timelines')
    poses=[compose(x,y) for x,y in zip(records[0]['poses'],records[1]['poses'])]
    a.output.mkdir(parents=True,exist_ok=False)
    for source,record in zip(sources,records):
        for part in record['poses'][0]['parts']:
            shutil.copy2(source/part['texture'],a.output/f"actor-{record['actor_index']}-{part['texture']}")
    (a.output/'poses.json').write_text(json.dumps({'mode':poses[0]['source'],
        'capture_times_seconds':records[0]['capture_times_seconds'],'poses':poses})+'\n')
    (a.output/'actor.json').write_text(json.dumps(poses[0])+'\n')
    print(f'Composed {len(poses)} synchronized paired frames in a shared captured coordinate space.')

if __name__=='__main__':main()
