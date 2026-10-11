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

Shared-coordinate research: extraction retains the derived actor-to-clip
matrix and compose_captured_pair.py cancels a common projection to place
synchronized geometry together. Synthetic checks verify transform direction,
UV preservation, input isolation, mismatched-frame rejection and singular
matrices. A 3-frame original capture renders both wrestlers in one view.
However, its derived relative transform differs from the independently stored
actor-root transform (maximum element error 57.398259 in the inspected frame)
and includes unexpected axis coupling. This is unresolved, possibly involving
pose/render timing or matrix interpretation. The paired compositor is an
offline diagnostic, not a validated world anchor or native combat feature.
Do not use its relative matrix for combat positioning until this discrepancy
is explained and verified against original displayed contact.

Matrix discrepancy diagnosis: pausing the original match allows pose and
submitted renderer matrices to settle. The inspected paused pair's relative
root disagreement falls to 0.0028258645, with per-joint camera spreads
0.0022438746 and 0.0019942132. This supports a pose/render timing mismatch
in moving snapshots rather than one consistent incorrect matrix convention.
The compositor now rejects root or per-joint matrix disagreement above 0.01;
paused paired capture passes. Moving captures cannot yet be treated as
validated rigid world poses. The earlier decoded hand/neck fitting experiment used deformation-matrix
translations, which are not anatomical pivots; its RMS 21.78 does not establish
a contact or root-motion result. See the resolved-pivot correction below.
Original paired move positioning remains unverified.

Original campaign follow-up: the local native Kane reached and opened the first
corridor door through ordinary movement, entered the adjacent NPC/ladder room,
triggered the original conversation, and displayed the original Code Key pickup.
Player/camera movement and original story scripting continue with the imported
model. This is not a full-level/combat completion claim; ladder animation,
original battle behavior and save/load with the replacement remain pending.
The read-only player probe now includes the original camera-mode mirror at
gState+0x29A and scene bytes +0x254..0x256 (N64 byte swizzling verified in tests).
Bounds, finite-coordinate, byte-order and read-only tests pass; the native
executable rebuild passes. The already-running playthrough still uses the older
probe, so live phase-field output is pending a later restart.


Optional direct animation-preview control: `HH_KANE_PREVIEW_BUTTON=1`, together
with `HH_KANE_PREVIEW_MODEL_PATH`, reads the original game's unused L button
(default keyboard E). A rising edge in exploration camera mode 2 starts one
preview. Holding it cannot loop or defer a request across a task/mode change.
The button path replaces file polling; keep both options unset for ordinary
play. The rebuilt source also cancels immediately when camera mode leaves 2;
unit checks verify cancellation and prevent resuming a consumed request.

Live isolated portable test (autosave Off; earlier playthrough preserved): the
exact-local 69-frame archive `kane-chokeslam-exact-displaylists` rendered in the
original first corridor. Holding E for five seconds logged exactly one
preview-to-idle transition; releasing and pressing again produced a second
preview-to-idle transition. Original position remained (-300,490.25,262.175781).
`Kane-L-Button-Native-Preview.mp4` records the second press: 180 frames at 30 fps
in 6.000109 wall-clock seconds, H.264 Constrained Baseline/yuv420p, 1280x720.
It is silent attacker-only geometry playback, without damage/opponent contact.
The camera-mode observer now works live: opening cutscene 0, first corridor 2.
The subsequent mode-exit cancellation refinement passed unit tests and native
rebuild; a live scene transition during an active preview remains untested.


Resolved-pivot correction: the original matrix builder resolves actual joint
pivots in pose slots 1..19 at +0x0C; the deformation-matrix translation is not
the joint origin. The private research wrapper now supports `--positions`.
`check_skeleton_reference.py` compares 57 pivot float words as well as 320
matrix words. Both actor slots passed bit-exact comparisons against the paused
reference; the earlier forward-walk capture also passed. No guest memory or
capture files were modified. New skeletal bakes retain finite
`joint_pivots_model_space` for all 69 frames per side. A corrected right-hand /
neck fit across frames 8..54 has RMS 6.4384 at 185 degrees; this geometric
experiment does not establish the original root track, grip window or damage.

Campaign control check: the opening ladder leads DOWN. Ordinary downward stick
input moved Kane from floor Y 490.25 to Y 400.25 and reached the lower code-key
door. A baseline run with the original player confirmed the ladder direction.
Kane still renders idle during ladder motion; a climbing pose is pending.
The original inventory KEY tab displays Code Key x1. The lower door prompts
for a code key; passing it, saving and original battles remain unverified.


Paired retarget preview: `compose_skeletal_pair.py` places the two decoded
69-frame clips together using an explicitly authored 180-degree defender yaw
and offset (-7,0,-61). This is separate from the captured-pair reconstruction
path and does not bypass its matrix-consistency gate. The full imported second
wrestler now visibly lifts and falls in inspected CPU frames 48, 55 and 68.
Original root tracking, precise contact and gameplay damage are not established.
The new compiler archive has 42 textured parts and 1,554 triangles per frame,
5,990,928 body bytes and 14,766 valid relocations; loader/canary checks pass
within the unchanged production 8 MiB bound. Authored transform direction, UV
preservation, unchanged inputs and rejection of bad/mixed source data pass.
Native first-corridor playback now passes: the optional L-button preview shows
both imported wrestlers through the lift and fall, then returns to idle. Player
position stayed (-300,490.25,131.080002). The silent native video records 180
frames at 30 fps in 6.000107 seconds, H.264 Baseline/yuv420p, 1280x720. Both
models are rendered through the player preview; the defender is not connected
to an original enemy, and no damage is applied.


Decoded walking follow-up: original animation 4468 (Generic 01) now bakes 20
distinct finite connected poses from the exact-local bind reference. The
921,408-byte archive has 2,140 valid relocations. Native first-corridor movement
selects this decoded walking cycle and returns to idle on release; original
player movement and camera follow continue. The helper prefers the new
`kane-walk-exact-displaylists` archive when present, retaining the earlier
reference archive as fallback. This changes walking playback only, not original
controls or ladder/crawl poses. Compiler reports now retain source mode and
animation identity rather than labelling every generated archive as a capture.
