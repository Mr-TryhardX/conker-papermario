"""Check all original finisher previews through the actual Windows 3D UI."""
from pathlib import Path
import json
import os
import subprocess
import time

root=Path(__file__).resolve().parents[1];os.environ['DISPLAY']=':101'
x='/workspace/toolchain/sysroot/usr/bin/xdotool';p=root/'dist/AnimatedDemo/AnimatedDemo-diagnostics.json'

def tap(key):subprocess.run([x,'key',key],check=True);time.sleep(.12)
def state():
    before=p.stat().st_mtime_ns if p.exists() else 0;tap('F12');end=time.monotonic()+5
    while time.monotonic()<end:
        try:
            if p.stat().st_mtime_ns!=before:return json.loads(p.read_text())
        except (FileNotFoundError,json.JSONDecodeError):pass
        time.sleep(.08)
    raise RuntimeError('No fresh diagnostics')
def finish():
    end=time.monotonic()+12
    while time.monotonic()<end:
        s=state()
        if s['animation']==0:return s
        time.sleep(.20)
    raise RuntimeError('Move animation never completed')

window=subprocess.check_output([x,'search','--name','Original Animated'],text=True).strip().splitlines()[-1]
subprocess.run([x,'windowfocus',window],check=True)
s=finish()
if s['practice']:tap('Escape')
s=state();assert not s['battle'];career=s['career'];tap('p');assert state()['practice']
seen=[]
for _ in range(6):
    who=state()['visual_fighter'];seen.append(who);tap('8');s=state();assert s['animation']>0 and s['action']==7
    time.sleep(.55);tap('F12');time.sleep(.12)
    subprocess.run(['ffmpeg','-loglevel','error','-i',str(root/'dist/AnimatedDemo/AnimatedDemo-screenshot.bmp'),'-frames:v','1','-y',str(root/'artifacts'/f'finisher-{who}.png')],check=True)
    s=finish();assert s['career']==career;assert s['audio_queued'];tap('Tab')
assert sorted(seen)==list(range(6))
tap('7');assert state()['action']==6;finish();tap('Escape');s=state();assert not s['battle'];assert s['career']==career
(root/'artifacts/practice-ui-check.json').write_text(json.dumps({'passed':True,'fighters':seen,'career_unchanged':True},indent=2))
print('PASS: six distinct fighter finisher previews, powerbomb preview, no career changes or item consumption.')
