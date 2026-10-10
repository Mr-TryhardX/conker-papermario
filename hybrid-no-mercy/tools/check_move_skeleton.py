#!/usr/bin/env python3
"""Exercise decoded paired move records through the verified original builder.

This is offline research, not a Hybrid Heaven combat hook. ROM-derived RAM and
generated functions stay local. Requires check_skeleton_reference.py first.
"""
import argparse, hashlib, json, math, struct, subprocess
from pathlib import Path
from decode_nomercy_animation import decode_frame

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=Path)
    parser.add_argument('--attacker',default='34EB')
    parser.add_argument('--defender',default='34EC')
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    base=args.capture.resolve().read_bytes()
    if len(base)!=0x800000:raise ValueError('Expected 8 MiB logical capture')
    records=[]
    for name in [args.attacker,args.defender]:
        if len(name)!=4 or any(c not in '0123456789ABCDEF' for c in name):
            raise ValueError('Expected four uppercase hexadecimal file IDs')
        data=(root/'asset-import/nomercy/files'/f'{name}.bin').read_bytes()
        if len(data)<102 or (len(data)-4)%98:raise ValueError('Incomplete move record')
        records.append([decode_frame(data[i:i+98]) for i in range(4,len(data),98)])
    if len(records[0])!=len(records[1]):raise ValueError('Paired timelines differ in length')
    work=root/'artifacts/move-skeleton-reference';work.mkdir(parents=True,exist_ok=True)
    executable=root/'artifacts/skeleton-reference/reference-skeleton'
    summaries=[]
    for name,frames in zip([args.attacker,args.defender],records):
        hashes=set()
        for frame in frames:
            # A fresh private snapshot per frame avoids carrying prior frames
            # into the check. Non-translated joints retain captured bindings.
            ram=bytearray(base)
            for joint in frame:
                offset=0xA1130+joint['joint']*124
                struct.pack_into('>3f',ram,offset,*joint['angles_degrees_xyz'])
                if joint['translation_xyz'] is not None:
                    struct.pack_into('>3f',ram,offset+12,*joint['translation_xyz'])
            scratch=work/'private-frame.ram';scratch.write_bytes(ram)
            result=subprocess.run([executable,scratch],stdout=subprocess.PIPE,check=True).stdout
            if len(result)!=1280 or not all(math.isfinite(v) for v in struct.unpack('<320f',result)):
                raise AssertionError('Invalid matrix output')
            hashes.add(hashlib.sha256(result).hexdigest())
        if len(hashes)<2:raise AssertionError('Move produced no distinct skeleton poses')
        summaries.append({'id':name,'frames':len(frames),'distinct_matrix_sets':len(hashes)})
    report={'result':'finite distinct skeleton poses for both equal-length timelines',
            'records':summaries,'scope':'Offline private No Mercy memory; no live combat or opponent binding verified'}
    (work/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
