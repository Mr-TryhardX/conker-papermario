#!/usr/bin/env python3
"""Decode angle/translation records traced to original func_800135D0.

This establishes semantic frame data; hierarchy matrices, skinning and root-motion
integration still require the original actor update routines.
"""
import json,struct,sys
from pathlib import Path

JOINT_MAP=[1,2,3,4,5,7,6,8,12,11,9,10,14,13,15,19,16,17,18]
NAMES=['pelvis','lower_abdomen','upper_body','neck','head','lower_left_leg',
       'upper_left_leg','left_foot','left_hand','left_fingers','lower_left_arm',
       'upper_left_arm','lower_right_leg','upper_right_leg','right_foot',
       'lower_right_arm','right_hand','right_fingers','upper_right_arm']

def signed_magnitude(value,bits):
    magnitude=value&((1<<bits)-1)
    return -magnitude if value&(1<<bits) else magnitude

def decode_frame(data):
    if len(data)!=98:raise ValueError('Expected complete 98-byte frame')
    offset=0;root=[0,0,0];joints=[]
    for index,name in enumerate(NAMES):
        if offset+4>len(data):raise ValueError('Truncated angle record')
        word=struct.unpack_from('>I',data,offset)[0];offset+=4
        angles=[((word>>shift)&1023)*0.3515625 for shift in (0,10,20)]
        flag=word>>30;translation=None
        if flag==2:
            if offset+4>len(data):raise ValueError('Truncated absolute translation')
            value=struct.unpack_from('>I',data,offset)[0];offset+=4
            translation=[signed_magnitude(value,10),signed_magnitude(value>>11,10),signed_magnitude(value>>22,9)]
            root=translation.copy()
        elif flag==3:
            if offset+3>len(data):raise ValueError('Truncated relative translation')
            translation=[root[i]+signed_magnitude(data[offset+i],7)*.5 for i in range(3)];offset+=3
        joints.append({'joint':JOINT_MAP[index],'name':name,'angles_degrees_xyz':angles,'translation_xyz':translation,'translation_flag':flag})
    if offset!=len(data):raise ValueError('Frame layout does not match traced joint stream')
    return joints

def run(root,out):
    manifest=json.loads((root/'manifest.json').read_text());out.mkdir(parents=True,exist_ok=True)
    animations={};frames_checked=0
    for entry in manifest['files']:
        if entry['type']!='AkiAnimation' or entry['status']!='extracted':continue
        data=(root/'files'/f"{entry['id']}.bin").read_bytes()
        if len(data)<4 or (len(data)-4)%98:raise ValueError('Incomplete animation')
        # Check every frame; only selected records are materialized for research.
        frames=[]
        for offset in range(4,len(data),98):
            frame=decode_frame(data[offset:offset+98]);frames_checked+=1
            if entry['id']=='2DD9' or 'Chokeslam' in entry['label'] or 'Powerbomb' in entry['label']:frames.append(frame)
        animations[entry['id']]={'label':entry['label'],'frames':(len(data)-4)//98}
        if frames:(out/f"{entry['id']}.json").write_text(json.dumps({'label':entry['label'],'frames':frames})+'\n')
    report={'source_function':'800135D0','joint_remap_address':'80047EF8','angle_degrees_per_unit':.3515625,
            'relative_translation_per_unit':.5,'joint_count':19,'frames_checked':frames_checked,
            'animations':animations,'status':'Angle and translation decoding; actor hierarchy playback pending'}
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Decoded semantic angles/translations in {frames_checked} frames across {len(animations)} animations')

if __name__=='__main__':run(Path(sys.argv[1]),Path(sys.argv[2]))
