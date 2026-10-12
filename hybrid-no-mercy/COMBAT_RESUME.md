# Combat work checkpoint

User preference: no updated demo until combat animation works; show a combat
video instead. Health HUD stays last. Earlier private demo remains unchanged.

Authored original-animation builder and geometry hooks compile on Linux.
Battle state/metadata, bounded snapshot, wrapper ordering and cursor policy
fixtures passed. Authentic No Mercy straight-punch (332F, 9 frames) and front
kick (333C, 12 frames) geometry were decoded with the original builder and
converted to native model archives; model loader checks passed for both.
Actual synchronized combat playback is NOT verified. No combat demo/video is
ready for delivery. New Windows hook build remains untested.

Local source: /workspace/hybrid-no-mercy. Main source delivery branch:
Mr-TryhardX/conker-papermario hybrid-career-lab. Never publish provided ROMs,
RAM snapshots, original generated C/.inc or model archives to the public repo.

Current original 8000F458 builder wrapper captures state+1C resource/cursor
after the exact original body. The authored hook caches matching player
state; configurable HH_KANE_PUNCH_RESOURCE was set to 0x190015E in an isolated
research process. Native logs do not show the 9-frame combat archive loading,
so this binding/cache path needs diagnosis. Root draw fields sometimes show
only a one-frame attack pulse; do not treat guard clips 0190:0038/0039 as
punches. Source-only hook files are experimental, not a working combat claim.

Original 8000E748 produces the source-keyframe position and interpolation
fraction using descriptor duration and speed. Next investigate observing its
output directly, preserving original execution, instead of guessing timing
from metadata. Descriptor +8 high halfword is a keyframe count, NOT a starting
frame; Metadata.first_frame naming needs correction. Duration low halfword
may be scaled by the original speed; current cursor policy is preliminary.

Research video artifacts/HybridHeaven-Original-Combat-Research-04.mp4 contains
50 seconds of original DEMO PLAY combat. Its .capture.json records monotonic
start_host_ms/end_host_ms for alignment with actor traces. At video 11.86 s
the label UPPER L PUNCH appears; root trace bank 0190 clip 015E at frame 5.
At 12.4 s label MID R PUNCH appears, so no kick binding has been asserted.
Original battle clips and snapshots stay local.

Preserve prior paused gameplay sessions. Current research executable run:
artifacts/kane-combat-cursor-research; log kane-combat-cursor-runtime.log;
traces kane-original-animation-update.jsonl, kane-combat-cursor-state.jsonl,
kane-combat-cursor-player.jsonl, kane-combat-cursor-motion.jsonl. Motion trace
cap 128 was filled with idle/walk transitions. Runtime snapshots are ephemeral
and live processes may not survive environment restoration. Build command:
source /workspace/toolchain/activate.sh; python3 tools/apply_native_integration.py;
cmake --build native/HybridHeaven/build-rendered -j 4.
