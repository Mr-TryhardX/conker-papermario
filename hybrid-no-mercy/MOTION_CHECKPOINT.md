# Kane locomotion and skeleton reference

This checkpoint extends the actual Hybrid Heaven character replacement. It
does not deliver the complete merged campaign or a new Windows game package.

The local game renderer now selects a ten-pose standing reference or a
twenty-pose walking loop according to original-player displacement. Walking
stops when position stops changing, including at collisions. Room/player
rebinding, invalid coordinates and teleports reset the motion state. Original
controls, collision and camera logic remain in charge of movement.

The walk is interpolated from an inspected original No Mercy two-player match
with the second controller neutral. It is captured vertex animation, not yet
playback of the decoded skeletal animation archive. Both sets retain the same
21 costume textures and 777-triangle topology. A previous live check verified
idle/walk switching and camera follow, but revealed reversed facing. The
generator now defaults to 180-degree yaw. A new live first-corridor check
verified forward-facing walking, a return to standing after release, and
original camera follow. Walking cadence still needs tuning.

The capture parser now follows the submitted F3DEX2 actor OSTask, including
segment mappings and nested lists. It stops at the task bounds and end commands
instead of reading stale calls beyond the submitted display list. An optional
second ordinary controller avoids CPU interruptions during research captures.

## Verified skeleton reference

`tools/check_skeleton_reference.py` recompiles the original `func_80012BE0`
and eight dependencies locally, runs them against a private copy of supplied
No Mercy RAM, and compares their output with the captured renderer matrices.
The forward-walk and earlier menu captures both reproduce all 20 joint
matrices (320 float words) bit for bit. This validates the reference routine;
it does not establish arbitrary animation playback or wrestling combat in
Hybrid Heaven. Guest address spaces are kept separate.

Generated original C, full RAM snapshots, ROMs, extracted texture sets and
compiled model archives remain local. Only authored tools, tests and reports
belong in the delivery repository.

## Reproduce in the prepared workspace

From `/workspace/hybrid-no-mercy`, activate the local toolchain:

```sh
source /workspace/toolchain/activate.sh
python3 tools/check_skeleton_reference.py artifacts/nomercy-forward-walk-last.ram
python3 tools/check_skeleton_reference.py artifacts/nomercy-kane-logical.ram
g++ -std=c++20 -Wall -Wextra -Werror tests/kane_motion_test.cpp -o artifacts/kane-motion-test
artifacts/kane-motion-test
python3 tests/kane_displaylist_test.py asset-import/kane-idle-displaylists/kane-displaylists.bin 10
python3 tests/kane_displaylist_test.py asset-import/kane-walk-displaylists/kane-displaylists.bin 20
artifacts/kane-model-test asset-import/kane-idle-displaylists/kane-displaylists.bin 10
artifacts/kane-model-test asset-import/kane-walk-displaylists/kane-displaylists.bin 20
/opt/codex/runtimes/codex-primary-runtime/dependencies/python/bin/python3 tests/guest_actor_test.py
cmake --build native/HybridHeaven/build-rendered -j4
```

All checks above passed in the current workspace. `kane-model-test` is the
existing loader test compiled with the N64ModernRuntime include directories.
The local launcher `tools/run_native_kane.sh` automatically selects the new
archives when present. It still needs the native graphics/audio environment
documented in the saved cloud startup instructions.

Next: verify facing and walking cadence in the first original corridor, then
connect authenticated No Mercy moves and paired opponent animations to the
original combat state. Native combat music, cheers, shop and CAW integration
and full campaign playthrough remain outstanding.

`dist/asset-import-checkpoint/Kane-Original-Corridor-Walking.mp4` is a
silent, accelerated diagnostic: 48 screenshots captured over 28.181 seconds
are shown over six seconds. It uses H.264 Constrained Baseline and yuv420p
for Windows compatibility. It is evidence of the local native test, not a
playable download or a frame-rate benchmark.

Cadence follow-up: the initial eight-unit walk cycle was too fast at the
observed corridor speed. The cycle now spans 80 movement units. A deterministic
check verifies quarter-cycle advances and wraparound over that distance,
and the native executable rebuild passed. Live cadence validation is pending;
the earlier video shows the initial faster cadence.

Latest live follow-up: the rebuilt 80-unit walking cycle rendered in the
original first corridor; W moved the player to (-300,490.25,191.719772),
original camera follow continued, and releasing W selected standing again.
This supersedes the earlier pending live-cadence check.

`python3 tools/check_move_skeleton.py artifacts/nomercy-forward-walk-last.ram`
checks original records 34EB and 34EC (Chokeslam from Hell attacker and
defender) through the verified matrix builder. Both have 69 frames, all
produce finite matrices, and all 69 matrix sets per side differ. This offline
check uses the same captured actor binding separately for each side; it does
not validate an opponent mesh, attachment transforms, paired contact, frame
interpolation, collision, hit timing, damage or playable combat.
