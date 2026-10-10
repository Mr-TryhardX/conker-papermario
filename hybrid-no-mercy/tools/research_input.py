#!/usr/bin/env python3
"""Normal N64 controller inputs for the isolated cloud research emulator."""
from pathlib import Path
import argparse,os,sys,time
path=Path(__file__).resolve().parents[1]/'artifacts/research-input.txt'
buttons={'right':1,'left':2,'down':4,'up':8,'start':16,'z':32,'b':64,'a':128,'c_right':256,'c_left':512,'c_down':1024,'c_up':2048,'r':4096,'l':8192}
axes={'stick_left':(-80,0),'stick_right':(80,0),'stick_up':(0,80),'stick_down':(0,-80)}
def write(mask=0,x=0,y=0,player=1):
 target=path if player==1 else path.with_name("research-input-p2.txt")
 if player not in (1,2):raise ValueError("Research player must be 1 or 2")
 temp=target.with_suffix('.next');temp.write_text(f'{mask:x} {x} {y}\n');os.replace(temp,target)
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--player',type=int,choices=[1,2],default=1)
 parser.add_argument('keys',nargs='*',choices=list(buttons)+list(axes))
 args=parser.parse_args()
 try:
  for key in args.keys:
   if key in buttons:write(buttons[key],player=args.player)
   else:write(0,*axes[key],player=args.player)
   time.sleep(.15);write(player=args.player);time.sleep(.7)
 finally:write(player=args.player)
if __name__=='__main__':main()
