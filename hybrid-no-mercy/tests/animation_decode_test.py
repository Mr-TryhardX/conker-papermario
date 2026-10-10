#!/usr/bin/env python3
import struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from decode_nomercy_animation import decode_frame,signed_magnitude
assert signed_magnitude(0x80,7)==0 and signed_magnitude(0xff,7)==-127
assert signed_magnitude(0x3ff,10)==1023 and signed_magnitude(0x7ff,10)==-1023
record=struct.pack('>II',(2<<30)|(1023<<20)|512,10|((1024|20)<<11)|((512|30)<<22))
for i in range(18):
    record+=struct.pack('>I',(3 if i<6 else 1)<<30)
    if i<6:record+=bytes([2,0x84,1])
assert len(record)==98
j=decode_frame(record)
assert [x['joint'] for x in j]==list(range(1,20))
assert j[0]['angles_degrees_xyz']==[180,0,359.6484375]
assert j[0]['translation_xyz']==[10,-20,-30]
assert j[1]['translation_xyz']==[11,-22,-29.5]
assert j[-1]['translation_xyz'] is None
for bad in [record[:-1],record+b'\0',bytes(98)]:
    try:decode_frame(bad)
    except ValueError:pass
    else:raise AssertionError('Invalid frame accepted')
print('PASS: traced angle units, sign-magnitude translations, relative roots and malformed frames.')
