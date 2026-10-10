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

Decoded-geometry follow-up: the original stream loader (func_80013234) writes
joints 1..19 sequentially; table 80047EF8 is update order, not storage order.
The earlier remap interpretation was wrong and produced separated limbs.
The decoder is corrected, and ten reference frames now match the original
loader AND decoder in their actual runtime slots, including unset translations.
All 152778 frames were regenerated locally in asset-import/decoded-animation-v2;
the earlier decoded-animation directory is retained but its joint labels are
obsolete. The authored geometry baker preserves per-vertex bone provenance
for shared vertex-cache slots and attaches local costume matrices to parents.
All 681 reference vertex bindings resolve.

The corrected offline Chokeslam from Hell attacker produces 69 connected
rendered poses (21 parts, 777 triangles). Its 2998224-byte native display-list
body and 7383 relocations pass archive and loader checks. The test allocation
fixture now covers the production loader's existing 8 MiB limit and verifies
canaries at the actual allocation boundary; 10/20/69-pose tests all pass.
The new silent H.264 preview plays these decoded geometry poses at a chosen
20 frames per second. It is an offline attacker-only preview, without an
opponent or live combat/damage/music. No full playable finisher is claimed.

Native one-shot follow-up: the optional preview model/request-file bridge now
plays the decoded 69-frame attacker archive in the actual first corridor,
then returns to standing. Task rebinding, cancellation, duplicate draws and
completion are tested. The native player probe remains at
(-300,490.25,262.175781) across preview/standing transitions. This is geometry
staging, without opponent contact, damage or new combat audio.
Kane-Native-Chokeslam-One-Shot.mp4 is silent H.264, 1280x720, 180 frames
at 30fps: capture duration 6.000232 seconds, video duration 6 seconds.
The bridge is disabled unless HH_KANE_PREVIEW_MODEL_PATH and
HH_KANE_PREVIEW_REQUEST_PATH are explicitly supplied. Change the request
file to a new positive uint32 token to start; zero cancels.

Exact-bind follow-up: extraction now retains original local vertex coordinates
so skeletal baking avoids inverse projection of rounded renderer matrices.
The active native recording uses the preceding world-inverse-bind archive;
exact-local attacker and defender bakes remain offline. Extraction, original
skeleton checks and baking now support actor indices 0..3 with a validated
0x1810 skeleton stride and 2852-byte pose-array stride. Slot 1's actual
Chris Benoit mesh extracts as 21 parts/777 triangles; all 20 matrices match
the original builder bit for bit, and record 34EC produces 69 distinct finite
defender poses. A second-actor synthetic check verifies vertex binding
isolation. Paired world anchors/contact still need original-game validation.

The paired skeleton checker now uses distinct captured actor slots by default
(attacker 0, defender 1), including their separate pose arrays. Both timelines
again produce 69 finite distinct matrix sets. The exact-local attacker archive
also compiles and passes native display-list and owned-loader checks at
2998224 bytes/7383 relocations; it has not replaced the active recorded build.

Paired capture follow-up: --actors 0 1 now captures both wrestlers from each
immutable guest snapshot with identical timestamps. Managers in a ring push
the second wrestler's draw entries beyond the old 128-word prefix (observed
132..177); candidate discovery now inspects 256 bounded words, validates
object/joint ownership and confirms parts through the submitted task replay.
A skeleton-object-pointer alternative was tested and rejected because those
objects do not necessarily match the current submitted draw lists. The retained
24-frame ordinary-controller grapple attempt contains 21 parts/777 triangles
per actor with synchronized hashes and finite geometry. Move identity, paired
contact and native combat remain unverified. Health HUD is explicitly last.
