#!/usr/bin/env python3
"""Steer an owned native test window using normal keyboard input and fresh probes.

No guest-memory writes, teleports, save edits, or gameplay-state overrides.
The original camera, collisions and interactions remain in control.
"""
import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import time


def latest(path):
    with path.open('rb') as stream:
        stream.seek(0, 2)
        stream.seek(max(0, stream.tell() - 8192))
        lines = stream.read().splitlines()
    for line in reversed(lines):
        try:
            record = json.loads(line)
            if all(k in record for k in ('frame', 'camera_mode', 'position', 'eye', 'target')):
                if not all(math.isfinite(v) for key in ('position', 'eye', 'target') for v in record[key]):
                    raise ValueError('Nonfinite probe vectors')
                return record
        except json.JSONDecodeError:
            continue  # An append can be observed before its final newline.
    raise ValueError('No complete player probe record')


def fresh_after(path, frame, advances=2, timeout=7):
    deadline = time.monotonic() + timeout
    seen = frame
    while time.monotonic() < deadline:
        record = latest(path)
        if record['frame'] > seen:
            seen = record['frame']
            advances -= 1
            if advances == 0:
                return record
        time.sleep(.05)
    raise TimeoutError('Player probe did not advance; stop input rather than reuse stale state')


def steer(args):
    window = str(args.window)
    subprocess.run(['xdotool', 'getwindowpid', window], check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['xdotool', 'windowfocus', window], check=True)
    subprocess.run(['xdotool', 'keyup', 'w', 'a', 's', 'd'], check=True)
    record = latest(args.probe)
    record = fresh_after(args.probe, record['frame'], advances=1)
    deadline = time.monotonic() + args.timeout
    try:
        while time.monotonic() < deadline:
            x, y, z = record['position']
            dx, dz = args.x - x, args.z - z
            distance = math.hypot(dx, dz)
            print(json.dumps({'frame': record['frame'], 'mode': record['camera_mode'],
                              'position': record['position'], 'distance': round(distance, 3)}), flush=True)
            if args.until_mode is not None and record['camera_mode'] == args.until_mode:
                return record
            if args.until_mode is None and distance <= args.radius:
                return record
            if record['camera_mode'] != 2:
                raise RuntimeError('Original game left field mode before the requested goal')
            fx, fz = record['target'][0] - record['eye'][0], record['target'][2] - record['eye'][2]
            length = math.hypot(fx, fz)
            if length < 1e-5:
                raise ValueError('Camera has no usable horizontal direction')
            fx, fz = fx / length, fz / length
            forward, left = dx * fx + dz * fz, dx * fz - dz * fx
            # Use the eight normal keyboard directions. Four-direction steering
            # zigzags across narrow doorway triggers when the camera is oblique.
            strongest = max(abs(forward), abs(left))
            threshold = strongest * math.tan(math.pi / 8)
            keys = []
            if abs(forward) >= threshold:
                keys.append('w' if forward > 0 else 's')
            if abs(left) >= threshold:
                keys.append('a' if left > 0 else 'd')
            subprocess.run(['xdotool', 'keydown', *keys], check=True)
            time.sleep(max(.12, min(args.step, distance / 65)))
            subprocess.run(['xdotool', 'keyup', *keys], check=True)
            # Wait for two distinct appended samples after key release; the
            # renderer's probe is asynchronous and its default stride is 30.
            released_frame = latest(args.probe)['frame']
            record = fresh_after(args.probe, released_frame)
        raise TimeoutError('Route timed out without reaching the original game state')
    finally:
        subprocess.run(['xdotool', 'keyup', 'w', 'a', 's', 'd'], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--window', required=True, type=int)
    parser.add_argument('--probe', required=True, type=Path)
    parser.add_argument('--x', required=True, type=float)
    parser.add_argument('--z', required=True, type=float)
    parser.add_argument('--radius', type=float, default=1.5)
    parser.add_argument('--until-mode', type=int, choices=range(9))
    parser.add_argument('--step', type=float, default=.45)
    parser.add_argument('--timeout', type=float, default=90)
    args = parser.parse_args()
    if args.window <= 0 or not all(math.isfinite(v) for v in (args.x, args.z, args.radius, args.step, args.timeout)):
        parser.error('A real window and finite parameters are required')
    if args.radius <= 0 or not .1 <= args.step <= 1 or not 1 <= args.timeout <= 180:
        parser.error('Invalid radius, step, or timeout')
    os.environ.setdefault('DISPLAY', ':102')
    try:
        print('Reached:', json.dumps(steer(args)), flush=True)
    except (ValueError, RuntimeError, TimeoutError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Input stopped: {error}\n')


if __name__ == '__main__':
    main()
