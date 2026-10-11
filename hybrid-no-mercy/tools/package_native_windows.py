#!/usr/bin/env python3
"""Assemble a local experimental package; never publish ROMs or imported assets."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--build', type=Path, default=ROOT / 'native/HybridHeaven/build-win-ready')
    parser.add_argument('--rom', type=Path, help='Optional supplied USA ROM, for this local package only')
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Choose a new output folder; existing packages and saves are preserved.')
    files = {name: args.build / name for name in
             ('HybridHeavenRecompiled.exe', 'SDL2.dll', 'dxcompiler.dll', 'dxil.dll', 'recompcontrollerdb.txt')}
    files.update({
        'models/kane-idle.bin': ROOT / 'asset-import/kane-idle-displaylists/kane-displaylists.bin',
        'models/kane-walk.bin': ROOT / 'asset-import/kane-walk-exact-displaylists/kane-displaylists.bin',
        'models/paired-preview.bin': ROOT / 'asset-import/chokeslam-paired-displaylists/kane-displaylists.bin',
    })
    if args.rom:
        files['hybridheaven.us.z64'] = args.rom
    missing = [str(p) for p in files.values() if not p.is_file()]
    if not (args.build / 'assets').is_dir():
        missing.append(str(args.build / 'assets'))
    if missing:
        parser.error('Missing package inputs:\n' + '\n'.join(missing))
    args.output.mkdir(parents=True)
    hashes = {}
    for name, path in files.items():
        dest = args.output / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
        hashes[name] = hashlib.sha256(dest.read_bytes()).hexdigest()
    shutil.copytree(args.build / 'assets', args.output / 'assets')
    (args.output / 'portable.txt').touch()
    baseline = ROOT / 'artifacts/kane-battle-isolated'
    for name in ('controls.json', 'general.json', 'sound.json', 'graphics.json'):
        value = json.loads((baseline / name).read_text())
        if name == 'graphics.json':
            value['api_option'] = 'Vulkan'
        elif name == 'general.json':
            value['autosave_mode'] = 'Off'
        (args.output / name).write_text(json.dumps(value, indent=2) + '\n')
    batch = '''@echo off
setlocal
cd /d "%~dp0"
set "HH_KANE_IN_GAME=1"
set "HH_KANE_MODEL_PATH=%~dp0models\\kane-idle.bin"
set "HH_KANE_WALK_MODEL_PATH=%~dp0models\\kane-walk.bin"
set "HH_KANE_PREVIEW_MODEL_PATH=%~dp0models\\paired-preview.bin"
set "HH_KANE_PREVIEW_BUTTON=1"
set "HH_AUTOSTART="
set "HH_ACTOR_DRAW_PROBE_PATH="
set "HH_PLAYER_PROBE_PATH="
set "HH_KANE_PREVIEW_REQUEST_PATH="
if exist "hybridheaven.us.z64" (
  HybridHeavenRecompiled.exe "hybridheaven.us.z64" > native-launch.log 2>&1
) else (
  HybridHeavenRecompiled.exe > native-launch.log 2>&1
)
if errorlevel 1 (
  echo The demo exited with an error. Please keep native-launch.log.
  pause
)
'''
    (args.output / 'Play-Kane.bat').write_bytes(batch.replace('\n', '\r\n').encode('ascii'))
    (args.output / 'README.txt').write_text('''EXPERIMENTAL NATIVE KANE BUILD

Extract the entire folder, then double-click Play-Kane.bat. Use Start Game
in the launcher. If prompted, select your extracted Hybrid Heaven USA ROM.
This runs as a Windows application. It does not open in Project64.
Use Windows 10/11 x64, a GPU/driver supporting Vulkan, and the Microsoft
Visual C++ 2015-2022 x64 Redistributable.

WASD moves; Space is A; left Shift is B; R is the original R button;
Enter is Start. E previews a fixed-placement paired wrestling animation
in field mode. That preview does not hit an enemy or apply damage.
Original controls, story, collision, enemies and damage remain in charge.
Kane's combat poses/finishers, No Mercy battle music, crowd cheers, native
shop/CAW coupling and full campaign validation are unfinished.

Automatic saves are off. Original in-game saving needs an original save
device/slot; a complete save/load cycle has not been validated here.
Keep the whole folder and native-launch.log when reporting a problem.
This package contains local imported game assets: keep it out of public Git.
''')
    (args.output / 'package-manifest.json').write_text(json.dumps({
        'experimental': True, 'windows_runtime_verified': False,
        'full_campaign_verified': False, 'files_sha256': hashes,
    }, indent=2) + '\n')
    print(f'Created local experimental package: {args.output}')


if __name__ == '__main__':
    main()
