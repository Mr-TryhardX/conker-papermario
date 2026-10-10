#!/usr/bin/env python3
"""Capture inspected original-game poses without overwriting earlier captures."""
import argparse,array,json,sys,time
from pathlib import Path
from extract_guest_actor import run

root=Path(__file__).resolve().parents[1]
def capture(out, frames, interval, label, actors=(0,)):
    if not actors or len(set(actors))!=len(actors) or any(a not in range(4) for a in actors):
        raise ValueError("Expected distinct actor indices 0..3")
    if not 1 <= frames <= 120 or not 0.01 <= interval <= 5:
        raise ValueError('Expected 1..120 poses and a 0.01..5 second interval')
    if not label.strip():raise ValueError('Describe the inspected animation being captured')
    # A fresh directory preserves both earlier motion and shared texture files.
    out.mkdir(parents=True,exist_ok=False)
    flag=root/'artifacts/guest-ram.flag';raw=root/'artifacts/nomercy-guest.ram'
    snapshot=root/'artifacts/current-pose.ram';poses={a:[] for a in actors};timestamps=[]
    destinations={a:out if actors==(0,) else out/f'actor-{a}' for a in actors}
    start=time.monotonic()
    for i in range(frames):
        flag.touch();deadline=time.monotonic()+3
        while flag.exists():
            if time.monotonic()>deadline:raise RuntimeError('Research snapshot not serviced')
            time.sleep(.01)
        data=raw.read_bytes()
        if len(data)!=0x800000:raise ValueError('Incomplete guest snapshot')
        values=array.array('I');values.frombytes(data)
        if sys.byteorder=='little':values.byteswap()
        snapshot.write_bytes(values.tobytes())
        captured_at=time.monotonic()-start
        for actor in actors:
            destination=destinations[actor]
            run(snapshot,destination,actor)
            poses[actor].append(json.loads((destination/'actor.json').read_text()))
        timestamps.append(captured_at)
        time.sleep(interval)
    for actor in actors:
        (destinations[actor]/'poses.json').write_text(json.dumps({'mode':label,
            'actor_index':actor,'capture_times_seconds':timestamps,'poses':poses[actor]})+'\n')
    if actors!=(0,):
        (out/'capture.json').write_text(json.dumps({'mode':label,'actors':list(actors),
            'frames':frames,'capture_times_seconds':timestamps,
            'synchronization':'Every actor in a frame uses the same immutable guest snapshot'})+'\n')
    print(f'Captured {frames} synchronized poses for {len(actors)} actor(s); snapshots remain local.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True,help='New directory; existing captures are refused')
    p.add_argument('--frames',type=int,default=40)
    p.add_argument('--interval',type=float,default=.075)
    p.add_argument('--label',required=True,help='Describe the original game scene/motion inspected')
    p.add_argument('--actors',type=int,nargs='+',choices=range(4),default=[0])
    a=p.parse_args();capture(a.output,a.frames,a.interval,a.label,tuple(a.actors))
