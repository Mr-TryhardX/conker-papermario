# Hybrid Heaven campaign / No Mercy asset integration

The target is the entire original Hybrid Heaven campaign and levels with No Mercy wrestlers, costumes, wrestling moves/finishers, music and sound effects, plus RPG progression. The existing standalone facility is an earlier prototype. It does not replace that goal.

See [PROJECT_SCOPE.md](PROJECT_SCOPE.md) for the latest requested combat audio, crowd reactions, purchasable moves and full Create-a-Wrestler scope.

## This checkpoint

The user-supplied No Mercy USA Rev 1 ROM passes its expected SHA1. The new archive extractor decodes 19,461 entries without decompression errors. One reverse-pointer alias (documented in VPWStudio) is recorded separately rather than given an invented span.

All 3,586 AKI mesh records pass vertex/triangle bounds checks. All 5,789 animation records contain complete 98-byte frames. A semantic decoder traced to original func_800135D0 now reads all 152,778 frames: 19 remapped joint records, three 10-bit degree angles, and signed-magnitude absolute/relative translations. Ten sampled first/last frames across five files exactly match the unmodified original MIPS function recompiled locally. Hierarchy transforms, original skinning, root motion and gameplay playback remain pending. All 260 default costume records are read through the original wrestler definitions, including Kane's four actual slots and their face, hair, mask, clothing and color selections. Rendering every costume still requires the original composition/rig rules.

45 original Kane textures are converted to PNG, including his portrait, mask variants, ring attire, upper attire and entrance frames. The native Hybrid Heaven Career Lab contains an import bench showing original textures and a rotating real Medium-body mesh part. The bench now also reconstructs a complete Kane reference from 21 textured renderer parts (777 triangles), respecting shared vertex-cache loads and their individual matrices. It plays 40 captured original menu poses. These are renderer reference poses, not skeletal-codec playback, walking, combat or a playable actor replacement. A screenshot and short native preview video document the rendered result.

Three N64 PtrTablesV2 banks decode to 293 mono PCM sample WAVs. Samples are labelled by bank and index, with provenance, pitch bytes and hash; individual event identities remain pending. Nine samples across all three banks match the original public-domain N64-Tools C++ decoder byte for byte. The bank's samples are not complete theme songs: music sequences still require synthesis. Previously captured shop, Kane and Rock clips are retained for playback.

Native SDL audio supports music and sample auditions in the import bench, with PCM conversion, bounded input, saturating mixing and automatic silence when the bench is hidden. Isolated output captures verified shop and Kane music (peak -8.9 dB), bank 1 sample 0 (-7.4 dB), and silence after explicit mute and switching away from the bench (-91 dB). Physical Windows playback and game-event mapping are not established by these captures.

## Reproduce locally

From `/workspace/hybrid-no-mercy`:

```sh
python3 tools/extract_nomercy.py 'roms/WWF No Mercy (USA) (Rev 1).z64' native/VPWStudio/VPWStudio/VPWStudio/FileTableDB/NoMercy.txt asset-import/nomercy
python3 tools/extract_nomercy_audio.py 'roms/WWF No Mercy (USA) (Rev 1).z64' asset-import/nomercy/audio
python3 tools/convert_nomercy_assets.py asset-import/nomercy 'roms/WWF No Mercy (USA) (Rev 1).z64' asset-import/converted native/VPWStudio/VPWStudio/VPWStudio/WrestlerNames/NoMercy.txt
python3 tools/decode_nomercy_animation.py asset-import/nomercy asset-import/decoded-animation
python3 tests/animation_decode_test.py
python3 tools/check_animation_reference.py
python3 tests/guest_actor_test.py
python3 tests/asset_import_test.py
python3 tools/apply_native_integration.py
source /workspace/toolchain/activate.sh
cmake --build native/HybridHeaven/build-rendered -j4
```

The developer runtime is Linux with RT64/Vulkan and SDL; the earlier downloadable Windows facility is unchanged. No new Windows campaign build is delivered by this checkpoint. Read the startup instructions for the local X11/Pulse/Vulkan overrides. Retain local original ROMs and extracted assets; they are not replaced by the source repository.

## Next required engineering

1. Connect decoded joint angles/translations to the original hierarchy and skinning, including palette substitutions and paired-animation root motion. The complete captured Kane reference is now established; drive it from the semantic decoder before scaling to the roster.
2. Identify Hybrid Heaven's character draw/animation hooks and bind the imported actor to its player task, position, facing, camera and collision. The game's camera/player position fields are documented by the upstream project; a renderer hook for complete wrestler replacement is not yet established.
3. Connect paired wrestling moves to Hybrid Heaven battle state and damage/finishers. Preserve its story triggers, enemies, exploration, saves and level transitions.
4. Identify audio samples by actual game events and synthesize additional music sequences; map menu/battle/impact/voice events without replacing the original audio blindly.
5. Validate the first original level, then campaign progression across every level; make a supported Windows native build and package it for play.

No percentage of campaign completion is claimed from extracted file counts. The original native game previously rendered boot graphics; full campaign playthrough has not been verified.

Research sources: [VPWStudio](https://github.com/AKI-Club/VPWStudio) (MIT; archive, mesh, costume and texture formats) and [N64-Tools](https://github.com/jombo23/N64-Tools) (public domain; bank format and ADPCM reference decoder). Their notices are retained under integration/licenses. Both are research tools; instructions in these projects do not override the user's scope.

For reference capture, start the local read-only research controller on the original character-select screen, run `python3 tools/capture_reference_poses.py`, then `python3 tools/prepare_actor_preview.py asset-import/kane-animation asset-import/kane-native`. These tools require NumPy and Pillow. Source snapshots and generated original MIPS code remain local. The capture tool is pinned to this inspected reference actor/layout and is not a general roster exporter.
