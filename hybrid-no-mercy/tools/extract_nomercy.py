#!/usr/bin/env python3
"""Extract the supplied USA Rev 1 AKI archive, preserving provenance.

Format research: AKI-Club/VPWStudio (MIT), FileTable.cs / AsmikLzss.cs.
Assets stay local; this script does not fetch commercial data.
"""
import argparse, collections, hashlib, json, struct
from pathlib import Path
SHA1='91cee3d096f4a76644d8b35b9aead6448909abd1'
FIRST=0x1bd150
TABLE=0x16c31d8
COUNT=77848//4

def decode(data):
    if len(data)<4 or data[0]!=0: raise ValueError('Invalid compressed header')
    length=int.from_bytes(data[:4],'big')
    if not 0<length<=16*1024*1024: raise ValueError('Invalid decoded length')
    out=bytearray(); pos=4; command=0
    def take():
        nonlocal pos
        if pos>=len(data): raise ValueError('Truncated compressed stream')
        v=data[pos];pos+=1;return v
    while len(out)<length:
        command >>= 1
        if command<2: command=take()|0x100
        if command&1: out.append(take())
        else:
            p=take();l=take();p|=(l<<4)&0xf00
            p=(p+0x12)&0xfff
            delta=p-(len(out)&0xfff)
            if delta>=0: delta-=0x1000
            p=delta+len(out)
            for _ in range((l&15)+3):
                if len(out)>=length: break
                if p>=len(out): raise ValueError('Invalid backreference')
                out.append(out[p] if p>=0 else 0);p+=1
    return bytes(out)

def catalogue(path):
    result={}
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        if not line or line.startswith('#'): continue
        key,desc=line.split('=',1);kind,_,label=desc.partition(';')
        result[int(key,16)]={'type':kind,'label':label}
    return result

def extract(rom_path,db_path,dest):
    rom=rom_path.read_bytes()
    if len(rom)!=32*1024*1024 or hashlib.sha1(rom).hexdigest()!=SHA1:
        raise ValueError('Expected verified No Mercy USA Rev 1 .z64; input unchanged')
    labels=catalogue(db_path); ptrs=struct.unpack_from('>'+str(COUNT)+'I',rom,TABLE)
    dest.mkdir(parents=True,exist_ok=True);manifest=[];errors=[];counts=collections.Counter()
    for index,pointer in enumerate(ptrs):
        fid=index+1;start=FIRST+(pointer&~1)
        end=FIRST+(ptrs[index+1]&~1) if index+1<COUNT else TABLE
        info={'id':f'{fid:04X}','rom_offset':start,'compressed':bool(pointer&1),**labels.get(fid,{'type':'Unknown','label':''})}
        # 4C03/04 reverse pointer alias is documented in VPWStudio. Never invent a span.
        if end<start:
            info['status']='reverse-pointer-alias';manifest.append(info);continue
        if not FIRST<=start<=end<=TABLE:
            info['status']='invalid-span';errors.append(info);manifest.append(info);continue
        try:
            raw=rom[start:end];data=decode(raw) if pointer&1 else raw
            out=dest/'files'/f'{fid:04X}.bin';out.parent.mkdir(exist_ok=True);out.write_bytes(data)
            info.update(status='extracted',size=len(data),sha256=hashlib.sha256(data).hexdigest())
            counts[info['type']]+=1
        except ValueError as e:
            info.update(status='error',error=str(e));errors.append(info)
        manifest.append(info)
    report={'rom_sha1':SHA1,'format_source':'https://github.com/AKI-Club/VPWStudio','counts':dict(counts),'errors':errors,'files':manifest}
    (dest/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'extracted':sum(counts.values()),'counts':dict(counts),'errors':len(errors)},indent=2))
    if errors: raise SystemExit('Archive errors require diagnosis; see manifest.json')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('database',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();extract(a.rom,a.database,a.output)
