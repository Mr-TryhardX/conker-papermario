"""Capture a labelled No Mercy music preview from the supplied ROM.

Open the game's music selector and highlight the desired title first. Each
capture starts with C-left (j) and records only that preview's PCM output.
"""
import os
from pathlib import Path
import subprocess
import sys
import time

root=Path(__file__).resolve().parents[1]
name=sys.argv[1]
if not name.replace('-','').replace('_','').isalnum():raise SystemExit('Use a simple track filename')
duration=float(sys.argv[2]) if len(sys.argv)>2 else 30
if not 5<=duration<=55:raise SystemExit('Capture duration must be 5–55 seconds')
os.environ['DISPLAY']=':100'
raw=root/'artifacts/shop.raw';start=raw.stat().st_size
flag=root/'artifacts/record.flag';flag.touch()
try:
 x='/workspace/toolchain/sysroot/usr/bin/xdotool'
 subprocess.run([x,'windowfocus','2097158','keydown','j'],check=True);time.sleep(.35)
 subprocess.run([x,'keyup','j'],check=True)
 time.sleep(duration)
finally:flag.unlink(missing_ok=True)
time.sleep(.1)
part=root/'artifacts'/f'{name}.raw'
with raw.open('rb') as f:f.seek(start);part.write_bytes(f.read())
dest=root/'assets/audio/entrances'/f'{name}.wav';dest.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['ffmpeg','-loglevel','error','-f','s16le','-ar','44100','-ac','2','-i',str(part),'-af',f'afade=t=in:d=0.1,afade=t=out:st={duration-1}:d=1','-y',str(dest)],check=True)
print(f'Captured {name}: {dest.stat().st_size} bytes')
