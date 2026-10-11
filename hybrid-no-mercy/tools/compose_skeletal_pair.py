#!/usr/bin/env python3
"""Compose two decoded geometry clips with an explicitly authored placement.

This is a retargeting preview, not reconstruction of the original root track.
It does not bypass the captured-pair consistency gate or apply game damage.
"""
import argparse, copy, json, math, shutil
from pathlib import Path
import numpy as np


def compose(attacker, defender, output, yaw_degrees=180.0, offset=(-7.0, 0.0, -61.0)):
    inputs=[Path(attacker),Path(defender)]
    docs=[json.loads((p/'poses.json').read_text()) for p in inputs]
    clips=[d['poses'] for d in docs]
    if len(clips[0])!=len(clips[1]) or not 1<=len(clips[0])<=120:
        raise ValueError('Paired clips require equal bounded frame counts')
    if not math.isfinite(yaw_degrees) or len(offset)!=3 or not np.isfinite(offset).all():
        raise ValueError('Invalid authored placement')
    c,s=math.cos(math.radians(yaw_degrees)),math.sin(math.radians(yaw_degrees))
    rotation=np.array([[c,0,-s],[0,1,0],[s,0,c]])
    frames=[];textures={}
    for left,right in zip(*clips):
        if left.get('actor_index',0)==right.get('actor_index',0):
            raise ValueError('Expected distinct source actor slots')
        if not left.get('snapshot_sha256') or left['snapshot_sha256']!=right.get('snapshot_sha256'):
            raise ValueError('Source bind captures differ')
        frame={'source':'Decoded skeletal pair with authored fixed placement; root/contact unverified',
               'parts':[],'paired_joint_pivots_model_space':[]}
        for actor,pose in enumerate([left,right]):
            pivots=np.array(pose.get('joint_pivots_model_space',[]),dtype=float)
            if pivots.shape!=(19,3) or not np.isfinite(pivots).all():
                raise ValueError('Expected 19 resolved finite joint pivots')
            if actor:pivots=pivots@rotation+offset
            frame['paired_joint_pivots_model_space'].append(pivots.tolist())
            for source in pose['parts']:
                if not np.array_equal(np.array(source['matrix']),np.eye(4)):
                    raise ValueError('Bake source geometry to model space first')
                part=copy.deepcopy(source)
                vertices=np.array(part['vertices'],dtype=float)
                if vertices.ndim!=2 or vertices.shape[1]!=5 or not np.isfinite(vertices).all():
                    raise ValueError('Invalid source vertex data')
                if any(len(f)!=3 or any(not isinstance(i,int) or i<0 or i>=len(vertices) for i in f) for f in part['faces']):
                    raise ValueError('Invalid triangle indices')
                if actor:vertices[:,:3]=vertices[:,:3]@rotation+offset
                part['vertices']=vertices.tolist()
                tex=Path(part['texture'])
                if tex.is_absolute() or '..' in tex.parts or not tex.parts:
                    raise ValueError('Invalid relative texture path')
                name=str(Path(f'actor-{actor}')/tex)
                textures[name]=inputs[actor]/tex
                part['texture']=name
                frame['parts'].append(part)
        frames.append(frame)
    output=Path(output);output.mkdir(parents=True,exist_ok=False)
    for name,source in textures.items():
        dest=output/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
    report={'mode':'Authored skeletal-pair placement preview',
            'source_animations':[d.get('animation') for d in docs],
            'defender_yaw_degrees':yaw_degrees,'defender_offset':list(offset),
            'original_root_track_verified':False,'contact_verified':False,'gameplay_damage':False}
    (output/'poses.json').write_text(json.dumps({**report,'poses':frames})+'\n')
    (output/'placement.json').write_text(json.dumps({**report,'frames':len(frames),
        'parts_per_frame':len(frames[0]['parts']),
        'triangles_per_frame':sum(len(p['faces']) for p in frames[0]['parts'])},indent=2)+'\n')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('attacker',type=Path);p.add_argument('defender',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--yaw-degrees',type=float,default=180)
    p.add_argument('--offset',type=float,nargs=3,default=(-7,0,-61))
    a=p.parse_args();print(json.dumps(compose(a.attacker,a.defender,a.output,a.yaw_degrees,a.offset),indent=2))
