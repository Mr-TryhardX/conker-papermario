#!/usr/bin/env python3
"""CPU render of imported reference geometry for local visual verification."""
import json,sys
from pathlib import Path
import numpy as np
from PIL import Image

def render(root,output,yaw=0):
    actor=json.loads((root/'actor.json').read_text());parts=[];all_points=[]
    c,s=np.cos(yaw),np.sin(yaw);rotation=np.array([[c,0,-s],[0,1,0],[s,0,c]])
    for p in actor['parts']:
        a=np.array(p['vertices']);v=np.c_[a[:,:3],np.ones(len(a))]@np.array(p['matrix']);v=v[:,:3]/v[:,3,None]
        v=v@rotation;parts.append((p,v,a[:,3:5]));all_points.extend(v)
    a=np.array(all_points);low,high=a.min(0),a.max(0);center=(low+high)/2
    size=640;scale=560/max(high[0]-low[0],high[1]-low[1]);image=np.full((size,size,4),[28,32,40,255],dtype=np.uint8);depth=np.full((size,size),-np.inf)
    for p,v,uv in parts:
        texture=np.array(Image.open(root/p['texture']).convert('RGBA'));h,w=texture.shape[:2]
        points=(v-center)*scale;points[:,0]+=size/2;points[:,1]=size/2-points[:,1]
        for face in p['faces']:
            tri=points[face];coords=uv[face];x0,y0=np.maximum(0,np.floor(tri[:,:2].min(0)).astype(int));x1,y1=np.minimum(size-1,np.ceil(tri[:,:2].max(0)).astype(int))
            if x1<x0 or y1<y0:continue
            x,y=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
            den=(tri[1,1]-tri[2,1])*(tri[0,0]-tri[2,0])+(tri[2,0]-tri[1,0])*(tri[0,1]-tri[2,1])
            if abs(den)<1e-8:continue
            b0=((tri[1,1]-tri[2,1])*(x-tri[2,0])+(tri[2,0]-tri[1,0])*(y-tri[2,1]))/den
            b1=((tri[2,1]-tri[0,1])*(x-tri[2,0])+(tri[0,0]-tri[2,0])*(y-tri[2,1]))/den;b2=1-b0-b1
            z=b0*tri[0,2]+b1*tri[1,2]+b2*tri[2,2]
            u=b0*coords[0,0]+b1*coords[1,0]+b2*coords[2,0];t=b0*coords[0,1]+b1*coords[1,1]+b2*coords[2,1]
            tex=texture[(np.floor(t*h).astype(int)%h),(np.floor(u*w).astype(int)%w)]
            block=depth[y0:y1+1,x0:x1+1];mask=(b0>=-1e-7)&(b1>=-1e-7)&(b2>=-1e-7)&(z>block)&(tex[:,:,3]>0)
            block[mask]=z[mask];image[y0:y1+1,x0:x1+1][mask]=tex[mask]
    Image.fromarray(image).save(output)

if __name__=='__main__':render(Path(sys.argv[1]),sys.argv[2],float(sys.argv[3]) if len(sys.argv)>3 else 0)
