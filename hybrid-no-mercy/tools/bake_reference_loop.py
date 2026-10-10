#!/usr/bin/env python3
"""Resample an inspected captured motion span into a grounded reference loop.

This interpolates captured vertices; it is not original skeletal-code playback.
Select matching cycle endpoints by visual inspection before using this tool.
"""
import argparse,copy,json,math,shutil
from pathlib import Path

def bake(source,output,start,end,count):
    record=json.loads((source/'poses.json').read_text())
    poses=record['poses'];times=record['capture_times_seconds']
    if len(poses)!=len(times) or not 0<=start<end<len(poses) or not 1<=count<=120:
        raise ValueError('Invalid source span or output frame count')
    times=times[start:end+1];poses=poses[start:end+1]
    if not all(math.isfinite(t) for t in times) or any(b<=a for a,b in zip(times,times[1:])):
        raise ValueError('Capture times must increase')
    first={p['bone']:p for p in poses[0]['parts']}
    for frame in poses:
        parts={p['bone']:p for p in frame['parts']}
        if set(parts)!=set(first):raise ValueError('Actor parts changed during the span')
        for bone,p in parts.items():
            reference=first[bone]
            if p['faces']!=reference['faces'] or len(p['vertices'])!=len(reference['vertices']) or p['texture']!=reference['texture']:
                raise ValueError('Topology or costume changed during the span')
            if any(not math.isfinite(v) for point in p['vertices'] for v in point):
                raise ValueError('Nonfinite source geometry')
    output.mkdir(parents=True,exist_ok=False)
    for p in first.values():shutil.copyfile(source/p['texture'],output/p['texture'])
    # The inspected last pose is close to the first; close at the exact first
    # geometry and sample without duplicating that endpoint in the loop.
    poses[-1]=poses[0]
    frames=[];duration=times[-1]-times[0]
    for i in range(count):
        t=times[0]+duration*i/count;j=0
        while j+1<len(times)-1 and times[j+1]<t:j+=1
        weight=(t-times[j])/(times[j+1]-times[j])
        left={p['bone']:p for p in poses[j]['parts']}
        right={p['bone']:p for p in poses[j+1]['parts']}
        frame=copy.deepcopy(poses[0])
        for p in frame['parts']:
            for v,a,b in zip(p['vertices'],left[p['bone']]['vertices'],right[p['bone']]['vertices']):
                v[:3]=[(1-weight)*a[k]+weight*b[k] for k in range(3)]
        floor=min(v[1] for p in frame['parts'] for v in p['vertices'])
        for p in frame['parts']:
            for v in p['vertices']:v[1]-=floor
        frames.append(frame)
    result={'mode':'Grounded vertex interpolation of inspected original-renderer reference poses',
            'source_capture':source.name,'source_frame_span':[start,end],
            'source_cycle_seconds':duration,'poses':frames}
    (output/'poses.json').write_text(json.dumps(result)+'\n')
    (output/'actor.json').write_text(json.dumps(frames[0])+'\n')
    print(f'Baked {count} reference frames from a {duration:.3f} second inspected span.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--start',type=int,required=True);p.add_argument('--end',type=int,required=True)
    p.add_argument('--frames',type=int,default=40)
    a=p.parse_args();bake(a.source,a.output,a.start,a.end,a.frames)
