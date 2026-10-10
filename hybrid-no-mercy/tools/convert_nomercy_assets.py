#!/usr/bin/env python3
"""Convert verified AKI records for native import; never infer skeleton bindings."""
from pathlib import Path
import json,struct,re,sys
from PIL import Image,ImageDraw

def mesh(data):
    if len(data)<8: raise ValueError('Short mesh header')
    scale,nv,nf,influence,x,y,z,ts=struct.unpack_from('>BBBBbbbB',data)
    nv&=127;nf&=127;end=8+8*nv+3*nf
    if end>len(data): raise ValueError('Truncated mesh')
    verts=[list(struct.unpack_from('>bbb5B',data,8+i*8)) for i in range(nv)]
    faces=[list(data[8+8*nv+i*3:11+8*nv+i*3]) for i in range(nf)]
    if any(i>=nv for f in faces for i in f): raise ValueError('Face outside vertex range')
    return {'scale':scale,'offset':[x,y,z],'influence':influence,'texture_size':[(ts>>4)*8 or 64,(ts&15)*8 or 64],'vertices':verts,'faces':faces}

def rgba5551(v): return tuple(((v>>n)&31)*255//31 for n in [11,6,1])+(255 if v&1 else 0,)

def texture(data,palette=None,bits=None):
    if data.startswith(b'TEX\0'):
        w,h=struct.unpack_from('>HH',data,4);bits=data[8];colorwidth=data[9];n=struct.unpack_from('>H',data,10)[0]
        if bits not in [4,8] or colorwidth!=2: raise ValueError('Unsupported TEX format')
        palette=data[16:16+n*2];pixels=data[16+n*2:]
    else:
        if len(data)<8 or palette is None or bits not in [4,8]: raise ValueError('Unsupported CI texture')
        w,h=data[0]+1,data[1]+1;pixels=data[8:]
    if not 0<w*h<=1024*1024 or len(pixels)*8<w*h*bits: raise ValueError('Texture bounds')
    colors=[rgba5551(v[0]) for v in struct.iter_unpack('>H',palette)]
    indices=list(pixels) if bits==8 else [n for b in pixels for n in [b>>4,b&15]]
    if any(i>=len(colors) for i in indices[:w*h]): raise ValueError('Texture palette index out of range')
    image=Image.new('RGBA',(w,h));image.putdata([colors[i] for i in indices[:w*h]]);return image

def run(root,rom,out,names):
    m=json.loads((root/'manifest.json').read_text());entries={e['id']:e for e in m['files']};out.mkdir(parents=True,exist_ok=True)
    meshes={};anims={};selected={};images=[]
    for e in m['files']:
        if e['status']!='extracted': continue
        d=(root/'files'/f"{e['id']}.bin").read_bytes()
        if e['type']=='AkiModel':
            model=mesh(d);meshes[e['id']]={'vertices':len(model['vertices']),'triangles':len(model['faces']),'label':e['label']}
            if 0x110a<=int(e['id'],16)<=0x111d: selected[e['id']]=model
        elif e['type']=='AkiAnimation':
            if len(d)<4 or (len(d)-4)%98: raise ValueError('Incomplete animation record')
            anims[e['id']]={'frames':(len(d)-4)//98,'header':d[:4].hex(),'label':e['label']}
        if 'Kane' in e['label'] and e['type'] in ['AkiTexture','Ci4Texture','Ci8Texture']:
            match=re.search(r'\|p:([0-9A-F]+)',e['label'])
            pal=(root/'files'/f'{match[1]}.bin').read_bytes() if match else None
            im=texture(d,pal,4 if e['type']=='Ci4Texture' else 8);im.save(out/f"{e['id']}.png");images.append((e['id'],im,e['label']))
    name_map={}
    for line in names.read_text().splitlines():
        if '=' not in line or line.startswith('#'):continue
        key,value=line.split('=',1);name_map[int(key.split('/')[0],16)]=value.split('|')[0]
    costume=(root/'files/0001.bin').read_bytes();r=rom.read_bytes();rows=[]
    fields=['body_type','skin_color','ring_attire','ring_color1','ring_color2','upper_attire','upper_color1','upper_color2','entrance_attire','entrance_color1','entrance_color2','entrance_weapon','gloves','gloves_color','tattoo','wristband','wristband_color','left_elbow','left_elbow_color','right_elbow','right_elbow_color','left_knee','left_knee_color','right_knee','right_knee_color','boots','boots_color1','boots_color2','head_shape','face','hair_type','hair_color','front_hair','facial_hair','mask','hat','hat_color','portrait']
    for i in range(260):
        a=struct.unpack_from('>HH4B5H2x',r,0x465b8+i*20);appear=a[-2]
        if appear*38+38>len(costume):raise ValueError('Appearance out of bounds')
        rows.append({'wrestler_id':a[1],'name':name_map.get(a[1],f'ID {a[1]:02X}'),'theme_id':a[2],'moves_file':f'{a[7]:04X}','params_file':f'{a[8]:04X}','appearance_index':appear,'costume':dict(zip(fields,costume[appear*38:(appear+1)*38]))})
    data={'rom_sha1':m['rom_sha1'],'meshes':meshes,'animations':anims,'costumes':rows,'preview_meshes':selected,'kane_textures':[i[0] for i in images],'integration_status':'Decoded assets; actor skeleton and gameplay bindings pending'}
    (out/'catalogue.json').write_text(json.dumps(data,indent=2)+'\n')
    sheet=Image.new('RGB',(640,((len(images)+7)//8)*105),(25,25,25));draw=ImageDraw.Draw(sheet)
    for i,(fid,im,_) in enumerate(images):
        x=(i%8)*80;y=(i//8)*105;im.thumbnail((76,80));sheet.paste(im,(x,y),im);draw.text((x,y+82),fid,fill='white')
    sheet.save(out/'kane-texture-sheet.png')
    print(f'Validated {len(meshes)} meshes, {len(anims)} animations, {len(rows)} costume records; converted {len(images)} Kane textures.')

if __name__=='__main__': run(*map(Path,sys.argv[1:]))
