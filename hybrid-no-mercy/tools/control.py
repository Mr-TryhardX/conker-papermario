"""Local keyboard driver for the emulators and Windows GUI smoke checks."""
import os
import subprocess
import sys
import time
from pathlib import Path

os.environ.setdefault("DISPLAY", ":100")
x = "/workspace/toolchain/sysroot/usr/bin/xdotool"
for key in sys.argv[1:]:
    subprocess.run([x, "keydown", key], check=True)
    time.sleep(0.35)
    subprocess.run([x, "keyup", key], check=True)
    time.sleep(0.7)
time.sleep(0.6)
root = Path(__file__).resolve().parents[1]
subprocess.run(["ffmpeg", "-loglevel", "error", "-f", "x11grab", "-video_size", "800x600", "-i", os.environ["DISPLAY"], "-frames:v", "1", "-y", str(root / "artifacts/nomercy.png")], check=True, stderr=subprocess.DEVNULL)
