#!/usr/bin/env python3
"""Compare Python decoding against the unmodified original MIPS routine.

Generated original code stays in local artifacts and is not distributed.
"""
import hashlib,json,math,struct,subprocess
from pathlib import Path
from decode_nomercy_animation import decode_frame

root=Path(__file__).resolve().parents[1];work=root/'artifacts/animation-reference';work.mkdir(parents=True,exist_ok=True)
rom=root/'roms/WWF No Mercy (USA) (Rev 1).z64'
if hashlib.sha1(rom.read_bytes()).hexdigest()!='91cee3d096f4a76644d8b35b9aead6448909abd1':raise ValueError('Wrong reference ROM')
(work/'symbols.toml').write_text('[[section]]\nname="decoder"\nrom=0x141D0\nvram=0x800135D0\nsize=0x290\nfunctions=[{name="func_800135D0",vram=0x800135D0,size=0x290}]\n')
(work/'recomp.toml').write_text('[input]\nrom_file_path='+json.dumps(str(rom))+'\nsymbols_file_path="symbols.toml"\noutput_func_path="generated"\n')
subprocess.run([root/'native/HybridHeaven/build-n64recomp/N64Recomp','recomp.toml'],cwd=work,check=True)
include=root/'native/HybridHeaven/lib/N64ModernRuntime/N64Recomp/include'
subprocess.run(['gcc','-O2','-I'+str(include),'-Igenerated','-c','generated/funcs_0.c','-o','generated.o'],cwd=work,check=True)
subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-I'+str(include),'-Igenerated',str(root/'tests/animation_reference_wrapper.c'),'generated.o','-lm','-o','reference-decoder'],cwd=work,check=True)
frames=[]
for fid in ['2DD9','2DDA','3000','3100','4000']:
    data=(root/f'asset-import/nomercy/files/{fid}.bin').read_bytes()
    if len(data)<102 or (len(data)-4)%98:raise ValueError('Wrong sample record')
    frames.extend([data[4:102],data[-98:]])
result=subprocess.run([work/'reference-decoder'],input=b''.join(frames),stdout=subprocess.PIPE,check=True)
if len(result.stdout)!=len(frames)*19*6*4:raise ValueError('Short original decoder output')
values=struct.unpack('<'+str(len(result.stdout)//4)+'f',result.stdout)
for f,frame in enumerate(frames):
    for i,joint in enumerate(decode_frame(frame)):
        got=values[f*114+i*6:f*114+i*6+6]
        want=joint['angles_degrees_xyz']+(joint['translation_xyz'] or [math.nan]*3)
        if not all((math.isnan(a)and math.isnan(b))or a==b for a,b in zip(got,want)):raise AssertionError((f,i,got,want))
report={'reference':'Unmodified func_800135D0 recompiled from supplied ROM','frames':len(frames),'joints_per_frame':19,'result':'exact float match including unset translations'}
(root/'artifacts/animation-decode-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(frames)} frames exactly match original MIPS decoder float results.')
