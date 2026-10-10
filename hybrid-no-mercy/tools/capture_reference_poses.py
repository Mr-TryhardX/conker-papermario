#!/usr/bin/env python3
"""Capture ordinary original-game poses from the read-only research controller."""
import array,json,time
from pathlib import Path
from extract_guest_actor import run

root=Path(__file__).resolve().parents[1]
out=root/'asset-import/kane-animation';out.mkdir(parents=True,exist_ok=True)
flag=root/'artifacts/guest-ram.flag';raw=root/'artifacts/nomercy-guest.ram'
snapshot=root/'artifacts/current-pose.ram';poses=[];timestamps=[]
start=time.monotonic()
for i in range(40):
    flag.touch();deadline=time.monotonic()+3
    while flag.exists():
        if time.monotonic()>deadline:raise RuntimeError('Research snapshot not serviced')
        time.sleep(.01)
    data=raw.read_bytes()
    if len(data)!=0x800000:raise ValueError('Incomplete guest snapshot')
    values=array.array('I');values.frombytes(data)
    # The local Mupen core stores guest words in little-endian host order.
    import sys
    if sys.byteorder=='little':values.byteswap()
    snapshot.write_bytes(values.tobytes());run(snapshot,out)
    poses.append(json.loads((out/'actor.json').read_text()));timestamps.append(time.monotonic()-start)
    time.sleep(.075)
(out/'poses.json').write_text(json.dumps({'mode':'Captured original menu reference poses, not decoded animation playback','capture_times_seconds':timestamps,'poses':poses})+'\n')
print(f'Captured {len(poses)} reference poses; original source snapshots remain local.')
