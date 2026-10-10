#!/usr/bin/env python3
"""Bake reference UV wrapping into an atlas for the UI's clamp-only sampler."""
import json,sys
from pathlib import Path
from PIL import Image

def run(root,out):
    j=json.loads((root/'poses.json').read_text());frames=j['poses']
    if not 0<len(frames)<=120:raise ValueError('Invalid pose count')
    out.mkdir(parents=True,exist_ok=True)
    for name in {p['texture']for p in frames[0]['parts']}:
        if '/' in name or '..' in name:raise ValueError('Invalid texture path')
        im=Image.open(root/name).convert('RGBA');w,h=im.size
        if w*h>65536:raise ValueError('Oversized actor material')
        atlas=Image.new('RGBA',(w*4,h*4))
        mirror=next(p.get('mirror',[False,False]) for p in frames[0]['parts'] if p['texture']==name)
        for y in range(4):
            for x in range(4):
                tile=im
                if mirror[0] and x&1:tile=tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                if mirror[1] and y&1:tile=tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                atlas.paste(tile,(x*w,y*h))
        atlas.save(out/name)
    for pose in frames:
        for part in pose['parts']:
            for v in part['vertices']:
                if not all(0<=v[k]<=4 for k in (3,4)):raise ValueError('UV outside reference atlas')
                v[3]/=4;v[4]/=4
    (out/'poses.json').write_text(json.dumps(j)+'\n')

if __name__=='__main__':run(Path(sys.argv[1]),Path(sys.argv[2]))
