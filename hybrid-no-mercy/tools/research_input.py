#!/usr/bin/env python3
"""Normal N64 controller inputs for the isolated cloud research emulator."""
from pathlib import Path
import os,sys,time
path=Path(__file__).resolve().parents[1]/'artifacts/research-input.txt'
buttons={'right':1,'left':2,'down':4,'up':8,'start':16,'z':32,'b':64,'a':128,'c_right':256,'c_left':512,'c_down':1024,'c_up':2048,'r':4096,'l':8192}
axes={'stick_left':(-80,0),'stick_right':(80,0),'stick_up':(0,80),'stick_down':(0,-80)}
def write(mask=0,x=0,y=0):
 temp=path.with_suffix('.next');temp.write_text(f'{mask:x} {x} {y}\n');os.replace(temp,path)
try:
 for key in sys.argv[1:]:
  if key in buttons:write(buttons[key])
  elif key in axes:write(0,*axes[key])
  else:raise ValueError('Unknown research controller button: '+key)
  time.sleep(.15);write();time.sleep(.7)
finally:write()
