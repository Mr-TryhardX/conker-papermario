"""Exercise the real Windows 3D UI under the local X11/Wine test session.

This uses key/mouse input and F12 diagnostic snapshots. It never edits saves or
grants progress. Run against a fresh test career for an initial chapter check.
"""
from pathlib import Path
import json
import os
import subprocess
import time

root=Path(__file__).resolve().parents[1]
os.environ['DISPLAY']=':101'
xdotool='/workspace/toolchain/sysroot/usr/bin/xdotool'
report=root/'dist/AnimatedDemo/AnimatedDemo-diagnostics.json'
events=[]

def tap(key):
    subprocess.run([xdotool,'key',key],check=True)
    time.sleep(.12)

def state():
    before=report.stat().st_mtime_ns if report.exists() else 0
    tap('F12')
    end=time.monotonic()+5
    while time.monotonic()<end:
        try:
            if report.stat().st_mtime_ns!=before:return json.loads(report.read_text())
        except (FileNotFoundError,json.JSONDecodeError):pass
        time.sleep(.08)
    raise RuntimeError('No fresh diagnostic snapshot')

def hold(key,seconds):
    subprocess.run([xdotool,'keydown',key],check=True)
    try:time.sleep(seconds)
    finally:subprocess.run([xdotool,'keyup',key],check=True)
    time.sleep(.12)

def navigate(x,z):
    for _ in range(35):
        s=state();dx=x-s['x'];dz=z-s['z']
        if abs(dx)<.35 and abs(dz)<.35:
            events.append({'position':[s['x'],s['z']]});return
        axis='x' if abs(dx)>.35 else 'z';delta=dx if axis=='x' else dz
        key=('d' if delta>0 else 'a') if axis=='x' else ('w' if delta>0 else 's')
        hold(key,min(1.3,max(.06,abs(delta)/3*.80)))
    raise RuntimeError(f'Could not navigate to {x},{z}: {state()}')

def settle():
    end=time.monotonic()+12
    while time.monotonic()<end:
        s=state()
        if s['animation']==0:return s
        time.sleep(.35)
    raise RuntimeError('Animation did not finish')

window=subprocess.check_output([xdotool,'search','--name','Original Animated'],text=True).strip().splitlines()[-1]
subprocess.run([xdotool,'windowfocus',window],check=True)
s=state();assert not s['battle'] and s['camera']==0
assert s['audio_queued'],'Audio player did not queue an output buffer'
routes=[[(0,1)],[(0,15.5),(-3,18)],[(3,24)],[(2,26),(2,35.5),(2,37)],[(-4,45)],[(3,48)]]
for encounter,route in enumerate(routes):
    for x,z in route:navigate(x,z)
    tap('e');s=state();assert s['battle'] and not s['practice']
    for turn in range(25):
        s=state()
        if not s['battle']:break
        assert s['hp']>0
        tap('3' if s['stamina']>=9 else 'g')
        s=settle()
    assert not s['battle'] and s['career']['cleared'][encounter],f'Encounter {encounter+1} not cleared'
    events.append({'cleared':encounter+1,'level':s['career']['roster'][5]['level'],'coins':s['career']['coins']})
    # Train Grapple with the earned points through the visible menu.
    tap('F3')
    points=s['career']['roster'][5]['points']
    for _ in range(points):
        subprocess.run([xdotool,'mousemove','880','250','click','1'],check=True);time.sleep(.12)
    tap('Escape')
    print(f'Cleared encounter {encounter+1}; career level {s["career"]["roster"][5]["level"]}',flush=True)
s=state();assert all(s['career']['cleared']);assert s['career']['wins']>=6
events.append({'complete':True,'career':s['career']})
(root/'artifacts/animated-ui-check.json').write_text(json.dumps(events,indent=2))
print('PASS: navigated the 3D chapter, six animated fights, earned rewards, trained stats, and queued music.',flush=True)
