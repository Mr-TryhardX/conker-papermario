#!/usr/bin/env python3
"""Safety checks for binary parsers, plus real imported-archive consistency."""
import sys,struct,json,wave
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from extract_nomercy import decode
from convert_nomercy_assets import mesh,texture
checks=0

def check(value):
 global checks
 assert value;checks+=1

def rejected(fn):
 global checks
 try:fn()
 except (ValueError,IndexError):checks+=1;return
 raise AssertionError('Malformed input accepted')

check(decode(bytes.fromhex('0000000c07414243eef6'))==b'ABCABCABCABC')
check(decode(bytes.fromhex('0000000307616263'))==b'abc')
rejected(lambda:decode(b'\0\0\0\x10'))
rejected(lambda:decode(b'\x01\0\0\x03abc'))
rejected(lambda:decode(bytes.fromhex('00ffffff')))
rejected(lambda:mesh(b'\0'*7))
valid=bytes([0,3,1,0,0,0,0,0])+bytes([0,0,0,0,0,255,255,255])*3+bytes([0,1,2])
check(len(mesh(valid)['vertices'])==3)
rejected(lambda:mesh(valid[:-1]))
rejected(lambda:mesh(valid[:-1]+bytes([3])))
header=bytes([1,0,0,16,0,0,1,0]);pal=bytes.fromhex('f80107c1')
check(texture(header+bytes([0x01]),pal,4).getpixel((0,0))==(255,0,0,255))
rejected(lambda:texture(header+bytes([0xff]),pal,4))
rejected(lambda:texture(header,pal,4))
p=Path(__file__).resolve().parents[1]/'asset-import/nomercy'
if p.exists():
 m=json.loads((p/'manifest.json').read_text());check(not m['errors'])
 check(sum(e['status']=='extracted' for e in m['files'])==19461)
 for e in m['files']:
  if e['type']=='AkiModel':check(bool(mesh((p/'files'/f"{e['id']}.bin").read_bytes())['vertices']))
  elif e['type']=='AkiAnimation':check(((p/'files'/f"{e['id']}.bin").stat().st_size-4)%98==0)
 for e in json.loads((p/'audio/manifest.json').read_text())['samples']:
  with wave.open(str(p/'audio'/e['bank']/f"{e['index']:03d}.wav")) as w:
   check(w.getnframes()==e['samples'] and w.getsampwidth()==2 and w.getnchannels()==1)
print(f'PASS: {checks} binary parser and real asset consistency checks.')
