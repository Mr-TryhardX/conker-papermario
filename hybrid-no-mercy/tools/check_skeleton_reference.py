#!/usr/bin/env python3
"""Reproduce the original skeleton builder using a private, owned RAM capture.

Generated ROM code and memory snapshots remain in local artifacts.
"""
import argparse, hashlib, json, math, struct, subprocess
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--actor-index',type=int,choices=range(4),default=0)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    ram = args.capture.resolve().read_bytes()
    if len(ram) != 0x800000:
        raise ValueError('Expected one logical big-endian 8 MiB capture')
    rom = root/'roms/WWF No Mercy (USA) (Rev 1).z64'
    if hashlib.sha1(rom.read_bytes()).hexdigest() != '91cee3d096f4a76644d8b35b9aead6448909abd1':
        raise ValueError('Wrong reference ROM')
    work = root/'artifacts/skeleton-reference'
    work.mkdir(parents=True, exist_ok=True)
    functions = {0x80001E08:0x344, 0x80010E50:0x174, 0x80010FC4:0x10,
                 0x80012BE0:0x368, 0x80034EF0:0x150, 0x80036460:0x1A0,
                 0x80038A00:0x48, 0x80038AC0:0x48, 0x80038B20:0xAC}
    entries = ',\n'.join(f'{{name="func_{a:08X}",vram=0x{a:X},size=0x{s:X}}}'
                         for a,s in functions.items())
    (work/'symbols.toml').write_text('[[section]]\nname="skeleton"\nrom=0x1050\nvram=0x80000450\nsize=0x3FF30\nfunctions=[\n'+entries+'\n]\n')
    (work/'recomp.toml').write_text('[input]\nrom_file_path='+json.dumps(str(rom))+'\nsymbols_file_path="symbols.toml"\noutput_func_path="generated"\n')
    subprocess.run([root/'native/HybridHeaven/build-n64recomp/N64Recomp', 'recomp.toml'], cwd=work, check=True)
    include = root/'native/HybridHeaven/lib/N64ModernRuntime/N64Recomp/include'
    subprocess.run(['gcc','-O2','-I'+str(include),'-Igenerated','generated/funcs_0.c',
                    str(root/'tests/skeleton_reference_wrapper.c'),'-lm','-o','reference-skeleton'],cwd=work,check=True)
    result = subprocess.run([work/'reference-skeleton',args.capture.resolve(),str(args.actor_index)],stdout=subprocess.PIPE,check=True)
    if len(result.stdout) != 20*16*4:
        raise ValueError('Short original skeleton output')
    maximum = 0.0
    for joint in range(20):
        original = struct.unpack_from('>16I',ram,0x91CC0+args.actor_index*0x1810+joint*0xDC+0x1C)
        rebuilt = struct.unpack_from('<16I',result.stdout,joint*64)
        for a,b in zip(original,rebuilt):
            x,y = struct.unpack('>f',struct.pack('>I',a))[0],struct.unpack('>f',struct.pack('>I',b))[0]
            if not math.isfinite(x) or not math.isfinite(y):
                raise AssertionError('Nonfinite reference matrix')
            maximum = max(maximum,abs(x-y))
        if original != rebuilt:
            raise AssertionError(f'Joint {joint} differs from captured matrix (max error {maximum})')
    pivots=subprocess.run([work/'reference-skeleton',args.capture.resolve(),str(args.actor_index),'--positions'],stdout=subprocess.PIPE,check=True).stdout
    if len(pivots)!=19*3*4:raise ValueError('Short original pivot output')
    for joint in range(1,20):
        original=struct.unpack_from('>3I',ram,0xA1130+args.actor_index*2852+joint*124+12)
        rebuilt=struct.unpack_from('<3I',pivots,(joint-1)*12)
        if original!=rebuilt:raise AssertionError(f'Joint {joint} pivot differs from capture')
        if not all(math.isfinite(x) for x in struct.unpack_from('<3f',pivots,(joint-1)*12)):
            raise AssertionError('Nonfinite reference pivot')
    report = {'reference':'Original func_80012BE0 and eight dependencies',
              'capture_sha256':hashlib.sha256(ram).hexdigest(),
              'actor_index':args.actor_index,'joints':20,'matrix_elements':320,'result':'bit-exact match',
              'joint_pivots':19,'pivot_elements':57,'pivot_result':'bit-exact match',
              'maximum_error':maximum,'address_space':'private No Mercy memory'}
    (work/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: all 20 original skeleton matrices match the captured renderer bit for bit.')
    print('PASS: all 19 resolved joint pivots match captured pose slots bit for bit.')

if __name__ == '__main__':
    main()
