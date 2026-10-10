#!/usr/bin/env python3
"""Decode No Mercy USA Rev 1 N64 PtrTablesV2 banks to local PCM WAVs.

Format/decoder reference: jombo23/N64-Tools (public domain),
N64AIFCAudio.cpp ReadAudioN64PtrWavetableV2/decode_8. No theme names
or sound-event labels are guessed from sample index. Music sequences
need synthesis and are distinct from these bank samples.
"""
from pathlib import Path
import array,hashlib,json,struct,sys,wave
from extract_nomercy import SHA1
BANKS=[('bank0',0x16f32a0,0x16f6f10),('bank1',0x1858030,0x185edb0),('bank2',0x1967410,0x1969880)]

def vadpcm(data,coefficients):
    last=[0]*8;out=array.array('h')
    for pos in range(0,len(data)-8,9):
        header=data[pos];shift=header>>4;pred=header&15
        if pred>=len(coefficients): raise ValueError('Invalid ADPCM predictor')
        c=coefficients[pred]
        for half in range(2):
            tmp=[]
            for b in data[pos+1+half*4:pos+5+half*4]:
                for n in [b>>4,b&15]:
                    n=(n if n<8 else n-16)<<shift
                    tmp.append(((n+32768)&65535)-32768)
            values=[]
            for i,t in enumerate(tmp):
                total=c[i]*last[6]+c[i+8]*last[7]
                for j in range(i): total+=tmp[j]*c[8+i-1-j]
                values.append(max(-32768,min(32767,((t<<11)+total)>>11)))
            last=values;out.extend(values)
    return out

def extract(path,dest):
    r=path.read_bytes()
    if hashlib.sha1(r).hexdigest()!=SHA1: raise ValueError('Verified USA Rev 1 ROM required')
    def word(pos):
        if not 0<=pos<=len(r)-4: raise ValueError('Pointer outside ROM')
        return struct.unpack_from('>I',r,pos)[0]
    report=[];dest.mkdir(parents=True,exist_ok=True)
    for name,ctl,tbl in BANKS:
        if r[ctl:ctl+len(b'N64 PtrTablesV2')]!=b'N64 PtrTablesV2': raise ValueError('Invalid bank magic')
        count=word(ctl+0x20);table=ctl+word(ctl+0x2c)
        if count>1024: raise ValueError('Bank count unreasonable')
        for i in range(count):
            offset=word(table+i*4)
            if not offset: continue
            entry=ctl+offset;start=tbl+word(entry);length=word(entry+4);book=ctl+word(entry+16)
            order=word(book);np=word(book+4)
            if order!=2 or not 1<=np<=16 or not 0<length<=16*1024*1024 or start+length>len(r):
                raise ValueError(f'Unsupported bank entry {name}/{i}')
            coeffs=[struct.unpack_from('>16h',r,book+8+p*32) for p in range(np)]
            pcm=vadpcm(r[start:start+length],coeffs)
            if sys.byteorder!='little': pcm.byteswap()
            folder=dest/name;folder.mkdir(exist_ok=True);output=folder/f'{i:03d}.wav'
            with wave.open(str(output),'wb') as w:
                w.setnchannels(1);w.setsampwidth(2);w.setframerate(22050);w.writeframes(pcm.tobytes())
            report.append({'bank':name,'index':i,'rom_offset':start,'encoded_bytes':length,'samples':len(pcm),'rate':22050,'base_pitch':r[ctl+word(ctl+0x24)+i],'fine_pitch':r[ctl+word(ctl+0x28)+i],'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'event':'unidentified'})
        print(name,count,'bank entries decoded',flush=True)
    (dest/'manifest.json').write_text(json.dumps({'rom_sha1':SHA1,'decoder_reference':'https://github.com/jombo23/N64-Tools','samples':report},indent=2)+'\n')
    print('Decoded',len(report),'bank samples. These are not synthesized full songs.')

if __name__=='__main__': extract(Path(sys.argv[1]),Path(sys.argv[2]))
