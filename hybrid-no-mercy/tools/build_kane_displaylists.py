"""Build local F3DEX2 model lists from inspected reference poses.

Uses the actual libultra command macros and MIPS relocations, rather than guessed
opcodes. Original assets and generated bulk data stay in the local workspace.
"""
import argparse,json,math,struct,subprocess,shutil
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
def run(args):
    result=subprocess.run(args,capture_output=True,text=True)
    if result.returncode:raise RuntimeError(result.stderr[:12000])
    return result.stdout

def build(source,out,frames=40,scale=10.0,gbi=None,yaw_degrees=180):
    poses=json.loads((source/'poses.json').read_text())['poses'][:frames]
    if not poses:raise ValueError('No poses')
    out.mkdir(parents=True,exist_ok=True)
    candidates=[ROOT.parent/'papermario-main/include/PR/gbi.h',ROOT.parent/'conker-papermario/papermario-main/include/PR/gbi.h']
    gbi=Path(gbi).resolve() if gbi else next((p for p in candidates if p.exists()),None)
    if gbi is None:raise RuntimeError('A complete F3DEX2 libultra gbi.h is required; the older Hybrid Heaven header deliberately rejects this mode.')
    lines=['#define F3DEX_GBI_2 1','#define F3DEX_GBI 1','#include <PR/ultratypes.h>',f'#include "{gbi}"','#include <ultra64.h>']
    textures={}
    for part in poses[0]['parts']:
        name=part['texture']
        if name in textures:continue
        im=Image.open(source/name).convert('RGBA');w,h=im.size
        if w*h*2>4096 or w&(w-1) or h&(h-1):raise ValueError('Texture cannot fit TMEM')
        symbol='texture_'+str(len(textures));textures[name]=(symbol,w,h)
        values=[((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|(a>=128) for r,g,b,a in im.get_flattened_data()]
        lines.append('u16 '+symbol+'[] __attribute__((aligned(8))) = {'+','.join(map(str,values))+'};')
    total=0
    for fi,pose in enumerate(poses):
        commands=['gsDPPipeSync()','gsSPClearGeometryMode(G_LIGHTING | G_CULL_BOTH)',
                  'gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH)',
                  'gsDPSetTextureLUT(G_TT_NONE)','gsDPSetCycleType(G_CYC_1CYCLE)',
                  'gsDPSetCombineMode(G_CC_MODULATERGBA, G_CC_MODULATERGBA)',
                  'gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2)',
                  'gsSPTexture(0xFFFF,0xFFFF,0,G_TX_RENDERTILE,G_ON)']
        for pi,part in enumerate(pose['parts']):
            tex,w,h=textures[part['texture']]
            mx,my=part['mirror'];verts=[]
            for face in part['faces']:
                if len(face)!=3:raise ValueError('Nontriangle')
                for vi in face:
                    x,y,z,u,v=part['vertices'][vi]
                    angle=math.radians(yaw_degrees)
                    rotated=(x*math.cos(angle)+z*math.sin(angle),y,-x*math.sin(angle)+z*math.cos(angle))
                    xyz=[round(c*scale) for c in rotated]
                    uv=[round(u*w*32),round(v*h*32)]
                    if any(c < -32768 or c>32767 for c in xyz+uv):raise ValueError('N64 Vtx overflow')
                    verts.append('{{{'+','.join(map(str,xyz))+'},0,{'+','.join(map(str,uv))+'},{255,255,255,255}}}')
            symbol=f'vertices_{fi}_{pi}';total+=len(verts)
            lines.append('Vtx '+symbol+'[] __attribute__((aligned(16))) = {'+','.join(verts)+'};')
            commands.append(f'gsDPLoadTextureBlock({tex},G_IM_FMT_RGBA,G_IM_SIZ_16b,{w},{h},0,{"G_TX_MIRROR" if mx else "G_TX_WRAP"},{"G_TX_MIRROR" if my else "G_TX_WRAP"},{int(math.log2(w))},{int(math.log2(h))},G_TX_NOLOD,G_TX_NOLOD)')
            for first in range(0,len(verts),30):
                n=min(30,len(verts)-first)
                commands.append(f'gsSPVertex({symbol}+{first},{n},0)')
                for t in range(0,n,6):
                    if t+6<=n:commands.append(f'gsSP2Triangles({t},{t+1},{t+2},0,{t+3},{t+4},{t+5},0)')
                    else:commands.append(f'gsSP1Triangle({t},{t+1},{t+2},0)')
        commands+=['gsDPPipeSync()','gsSPEndDisplayList()']
        lines.append(f'Gfx pose_{fi}[] __attribute__((aligned(8))) = {{'+','.join(commands)+'};')
    (out/'model.c').write_text('\n'.join(lines))
    include=ROOT/'native/HybridHeaven/lib/hybridheaven'
    run(['clang','-target','mips','-mips2','-mabi=32','-G0','-mno-abicalls','-Wno-incompatible-library-redeclaration','-Werror=excess-initializers','-nostdinc','-D_LANGUAGE_C','-DMIPS','-I',str(include/'include'),'-I',str(include/'libultra/include'),'-I',str(include/'libultra/include/PR'),'-c',str(out/'model.c'),'-o',str(out/'model.o')])
    (out/'model.ld').write_text('SECTIONS { .data 0 : { *(.data*) } /DISCARD/ : { *(.text*) *(.bss*) *(.reginfo*) *(.MIPS.abiflags*) *(.comment*) } }')
    run(['ld.lld','-T',str(out/'model.ld'),'--emit-relocs',str(out/'model.o'),'-o',str(out/'model.elf')])
    run([shutil.which('llvm-objcopy') or shutil.which('llvm-objcopy-19') or shutil.which('mips-linux-gnu-objcopy'),'-O','binary','--only-section=.data',str(out/'model.elf'),str(out/'model.raw')])
    symbols={}
    for line in run(['llvm-nm',str(out/'model.elf')]).splitlines():
        bits=line.split()
        if len(bits)==3:symbols[bits[2]]=int(bits[0],16)
    offsets=[symbols[f'pose_{i}'] for i in range(len(poses))]
    relocs=[]
    for line in run(['llvm-readelf','-r',str(out/'model.elf')]).splitlines():
        if 'R_MIPS_32' in line:relocs.append(int(line.split()[0],16))
    data=(out/'model.raw').read_bytes()
    for r in relocs:
        if r%4 or r+4>len(data) or struct.unpack_from('>I',data,r)[0]>=len(data):raise ValueError('Invalid model relocation')
    header=struct.pack('<4sIIII',b'KNDL',1,len(poses),len(data),len(relocs))
    metadata=struct.pack('<'+'I'*(len(offsets)+len(relocs)),*(offsets+relocs))
    (out/'kane-displaylists.bin').write_bytes(header+metadata+data)
    report={'frames':len(poses),'parts_per_frame':len(poses[0]['parts']),'triangles_per_frame':sum(len(p['faces']) for p in poses[0]['parts']),'vertices_total':total,'body_bytes':len(data),'relocations':len(relocs),'vertex_scale':scale,'yaw_degrees':yaw_degrees,'source':'Original-renderer reference poses; gameplay animation integration remains pending'}
    (out/'displaylist-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=ROOT/'asset-import/kane-animation');p.add_argument('--output',type=Path,default=ROOT/'asset-import/kane-displaylists');p.add_argument('--frames',type=int,default=40);p.add_argument('--scale',type=float,default=10);p.add_argument('--gbi',type=Path,help='Complete libultra F3DEX2 gbi.h; auto-detects the local Paper Mario header')
    p.add_argument('--yaw-degrees',type=float,default=180,help='Match No Mercy forward to the Hybrid Heaven root model axis')
    a=p.parse_args();print(json.dumps(build(a.source,a.output,a.frames,a.scale,a.gbi,a.yaw_degrees),indent=2))
