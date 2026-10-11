"""Validate the compiled N64 commands, not just the generator's source."""
import struct,sys
from pathlib import Path
p=Path(sys.argv[1]);raw=p.read_bytes()
magic,version,frames,size,relocs=struct.unpack_from('<4sIIII',raw)
expected_frames=int(sys.argv[2]) if len(sys.argv)>2 else 40
expected_triangles=int(sys.argv[3]) if len(sys.argv)>3 else 777
expected_parts=int(sys.argv[4]) if len(sys.argv)>4 else 21
assert magic==b'KNDL' and version==1 and 1<=frames<=120 and frames==expected_frames
metadata=struct.unpack_from('<'+'I'*(frames+relocs),raw,20)
body=raw[20+4*(frames+relocs):];assert len(body)==size
for r in metadata[frames:]:
 assert r%4==0 and r+4<=size and struct.unpack_from('>I',body,r)[0]<size
triangles=0
for root in metadata[:frames]:
 cache=set();count=0;loads=0;images=0;ended=False
 for pos in range(root,min(size,root+5000*8),8):
  w0,w1=struct.unpack_from('>II',body,pos);op=w0>>24
  if op==0xDF:ended=True;break
  if op==0x01:
   n=(w0>>12)&255;end=(w0>>1)&127;start=end-n
   assert 0<n<=32 and 0<=start<end<=32 and w1%16==0 and w1+16*n<=size
   cache=set(range(start,end));loads+=1
  elif op in (5,6):
   for word in ([w0] if op==5 else [w0,w1]):
    indices=[(word>>s)&255 for s in (16,8,0)]
    assert all(v%2==0 and v//2 in cache for v in indices)
    assert len(set(indices))==3
    count+=1
  elif op==0xFD:
   assert ((w0>>21)&7)==0 and ((w0>>19)&3)==2 and w1<size # RGBA16
   images+=1
 assert ended and count==expected_triangles and images==expected_parts and loads>expected_parts
 triangles+=count
print(f'Native display lists: {frames} frames, {triangles} triangles, {relocs} valid relocations; vertex caches and texture commands passed.')
