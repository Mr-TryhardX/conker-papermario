#!/usr/bin/env python3
"""Capture a local X11 window with XGetImage and a Windows-compatible encoder.

Optional trigger writes an atomic numeric renderer-research request. No game
memory, controller input, saves or external computer files are modified.
"""
import argparse, ctypes as C, ctypes.util, json, os, subprocess, time
from pathlib import Path

class XImage(C.Structure):
    _fields_=[('width',C.c_int),('height',C.c_int),('xoffset',C.c_int),('format',C.c_int),
              ('data',C.c_void_p),('byte_order',C.c_int),('bitmap_unit',C.c_int),
              ('bitmap_bit_order',C.c_int),('bitmap_pad',C.c_int),('depth',C.c_int),
              ('bytes_per_line',C.c_int),('bits_per_pixel',C.c_int),
              ('red_mask',C.c_ulong),('green_mask',C.c_ulong),('blue_mask',C.c_ulong)]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('output',type=Path);p.add_argument('--display',default=':102')
    p.add_argument('--window',type=lambda x:int(x,0),required=True)
    p.add_argument('--frames',type=int,default=180);p.add_argument('--fps',type=int,default=30)
    p.add_argument('--trigger-file',type=Path);p.add_argument('--trigger-value',type=int)
    p.add_argument('--trigger-frame',type=int,default=30)
    a=p.parse_args()
    if not 1<=a.frames<=3000 or not 1<=a.fps<=60 or a.output.exists():
        raise ValueError('Use a new output file and bounded frame count/rate')
    if a.trigger_file and (a.trigger_value is None or not 0<a.trigger_value<=0xffffffff or not 0<=a.trigger_frame<a.frames):
        raise ValueError('Trigger requires a nonzero uint32 token and valid frame')
    lib=C.CDLL(ctypes.util.find_library('X11'))
    lib.XOpenDisplay.argtypes=[C.c_char_p];lib.XOpenDisplay.restype=C.c_void_p
    lib.XCloseDisplay.argtypes=[C.c_void_p]
    lib.XGetGeometry.argtypes=[C.c_void_p,C.c_ulong,C.POINTER(C.c_ulong),
        C.POINTER(C.c_int),C.POINTER(C.c_int),C.POINTER(C.c_uint),C.POINTER(C.c_uint),C.POINTER(C.c_uint),C.POINTER(C.c_uint)]
    lib.XGetImage.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_int,C.c_uint,C.c_uint,C.c_ulong,C.c_int]
    lib.XGetImage.restype=C.POINTER(XImage);lib.XDestroyImage.argtypes=[C.POINTER(XImage)]
    display=lib.XOpenDisplay(a.display.encode())
    if not display:raise RuntimeError('X11 display unavailable')
    proc=None
    try:
        root=C.c_ulong();x=C.c_int();y=C.c_int();w=C.c_uint();h=C.c_uint();border=C.c_uint();depth=C.c_uint()
        if not lib.XGetGeometry(display,a.window,C.byref(root),C.byref(x),C.byref(y),C.byref(w),C.byref(h),C.byref(border),C.byref(depth)):
            raise RuntimeError('Window geometry unavailable')
        if not 1<=w.value<=4096 or not 1<=h.value<=4096:raise ValueError('Invalid window size')
        a.output.parent.mkdir(parents=True,exist_ok=True)
        proc=subprocess.Popen(['ffmpeg','-hide_banner','-loglevel','error','-f','rawvideo','-pixel_format','bgr0',
            '-video_size',f'{w.value}x{h.value}','-framerate',str(a.fps),'-i','pipe:0',
            '-c:v','libx264','-threads','2','-preset','veryfast','-profile:v','baseline',
            '-pix_fmt','yuv420p','-movflags','+faststart','-an',str(a.output)],stdin=subprocess.PIPE)
        start=time.monotonic()
        for frame in range(a.frames):
            if a.trigger_file and frame==a.trigger_frame:
                tmp=a.trigger_file.with_suffix('.capture-next');tmp.write_text(str(a.trigger_value)+'\n');os.replace(tmp,a.trigger_file)
            capture=lib.XGetImage(display,a.window,0,0,w.value,h.value,C.c_ulong(-1).value,2)
            if not capture:raise RuntimeError('Window capture failed')
            try:
                im=capture.contents
                if (im.bits_per_pixel,im.byte_order,im.bytes_per_line,im.red_mask,im.green_mask,im.blue_mask)!=(32,0,w.value*4,0xff0000,0xff00,0xff):
                    raise RuntimeError('Unsupported X11 pixel layout')
                proc.stdin.write(C.string_at(im.data,im.bytes_per_line*h.value))
            finally:lib.XDestroyImage(capture)
            time.sleep(max(0,start+(frame+1)/a.fps-time.monotonic()))
        elapsed=time.monotonic()-start
        proc.stdin.close()
        if proc.wait()!=0:raise RuntimeError('Video encoder failed')
        report={'frames':a.frames,'fps':a.fps,'captured_seconds':elapsed,'video_seconds':a.frames/a.fps,
                'width':w.value,'height':h.value,'timing':'nominal capture schedule; compare captured and video durations'}
        a.output.with_suffix('.capture.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2))
    finally:
        if proc and proc.poll() is None:
            if proc.stdin and not proc.stdin.closed:proc.stdin.close()
            proc.terminate();proc.wait()
        lib.XCloseDisplay(display)

if __name__=='__main__':main()
