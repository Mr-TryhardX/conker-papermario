# Read-only No Mercy runtime research

`research_controller.c` is a local Mupen64Plus 2.6 input plugin. It supplies ordinary button/stick state from `MERCY_RESEARCH_INPUT` and can capture only the core's guest RDRAM through `DebugMemGetPointer(1)` when `MERCY_CAPTURE_RAM_FLAG` exists, writing `MERCY_CAPTURE_RAM_PATH`. It does not read host memory or edit game state. Compile using the bundled `m64p_plugin.h`/`m64p_common.h` headers with `-O2 -Wall -Wextra -Werror -fPIC -shared -ldl`; build a new output and stop the research emulator before replacing a loaded plugin. Keep snapshots and original game saves local.

The API version is 0x020100, required by this core. The SDL swap preload used earlier stalled rendering; use the controller debugger path instead. Ordinary input and a complete 8 MiB guest snapshot were verified in the local emulator. `research_input.py` atomically writes inputs and returns to neutral after each press.

Verified menu path after boot: Start; L selects Multi Play; A enters; A selects Exhibition; Start accepts Single Match. On Superstars 1, Down then Right highlights Kane; A confirms player 1. In the local reference both players were Kane, showing different attire variants. Kane's full original 3D actor rendered in No Mercy. This verifies a reference only, not an imported actor in Hybrid Heaven.

The resident ROM/VMA relation is ROM = VMA - 0x80000000 + 0xC00. The resident renderer includes func_800032A4 -> func_80002FA4; the draw-object pointer table is 0x80061D40. Standard F3DEX2 display lists were observed. Original compact mesh byte prefixes were not found directly in the rendered actor snapshot; conversion/loading must be resolved rather than assuming extracted meshes are laid out verbatim in runtime RAM. A frame streaming routine around func_80011340 uses 98-byte records; semantic skeletal decoding remains pending.

Hybrid Heaven upstream camera research documents D_801BBBF0 + 0xE0 as player task pointer, task + 0x2C as data, world XYZ at +4/+8/+C, facing at +0x12. These are research leads; no player RAM modification or speculative actor injection is implemented.

## Verified renderer and animation findings

The actor renderer loads a shared vertex cache in chunks using opcode 0x01. Count is (w0 >> 12) & 255; end slot is (w0 >> 1) & 127. Slot start is end minus count. DC38000E force-matrix records select a combined 16.16 matrix for each chunk. For example the chest loads 26 vertices into slots 10..35, then ten neighboring-bone vertices into 0..9. Treating the object vertex pointer as slot zero caused stretched triangles and joint seams. Replaying these loads reconstructed a clean complete Kane: 21 textured parts and 777 triangles. The native preview plays 40 captured reference poses and does not yet drive the actor from the original skeletal codec.

Original func_800135D0 (ROM 0x141D0, size 0x290) decodes three unsigned 10-bit angle components, low-to-high X/Y/Z, multiplied by 0.3515625 degrees. Bit 31 enables a translation; with bit 30 set it reads three sign-magnitude bytes, multiplied by 0.5 and added to the shared absolute translation. Without bit 30 it reads a packed 32-bit sign-magnitude translation: X=10 magnitude bits + sign, Y=10+sign, Z=9+sign, and updates the shared translation. With bit 31 clear it leaves translation unset. The caller starts at joint 1 and stops before 20; Original func_80013234 stores stream records sequentially in runtime joints 1..19. ROM table 0x80047EF8 orders dependent updates; the earlier interpretation as a stream remap was incorrect. All 152,778 frames decode, but original hierarchy transforms and interpolation remain separate work. Ten sampled first/last frames exactly match outputs from the unmodified MIPS function recompiled by N64Recomp; tools/check_animation_reference.py reproduces the comparison without publishing original generated code.

## Hybrid Heaven player binding research

The original native game boots through New Game → Game Start and renders the
opening story cinematic. This is verification of the original runtime, not a
playable Kane campaign.

`integration/hh_player_probe.hpp` adds an optional, read-only observer to RT64's
`send_dl`. Set `HH_PLAYER_PROBE_PATH` to an absolute local JSONL output path before
launch. Without that variable the observer does nothing. Every 30 submitted display
lists it reads the documented gState+0xE0 task → +0x2C data chain and the camera
pointer at 0x801BBCD8 → +0x2C parameters. It records candidate position, eye/target,
and twelve raw task words. Every structure and float read is bounded to the 8 MiB
guest RAM range; nonfinite coordinates are rejected. It neither writes guest memory
nor saves full RAM/ROM/assets. Build validation and synthetic bounds/pointer/finite/
read-only tests pass (`g++ -std=c++17 -Wall -Wextra -Werror tests/player_probe_test.cpp`).
The live observer captured finite camera values during title/intro transitions.
Those pointers also exist on the title screen, so pointer validity alone must NOT
be used as a gameplay gate or proof of player movement. Live controlled movement
and the final tightened-span build's observer still need verification.

Traced original assembly identifies the draw path more precisely:

- `func_80124AF0_504200` copies the controller task's +0x24 child into gState+0xE0.
- `func_800058DC_64DC` sets the task's +0x1C update callback. The scheduler
  `func_80005270_5E70` calls callbacks at +0x18, +0x1C and +0x20. Thus these fields
  must not be confused with mesh pointers or the display-list emitter.
- Draw kind is u16 task+0x2A. `func_800069A8_75A8` dispatches it using
  `jtbl_8004C420_4D020`. Kind 1 invokes the perspective camera emitter 0x80007DE4;
  kind 6 goes through 0x80006A2C to `func_8000C768_D368` (kind 12 shares that emitter).
- Title/intro candidate task word +0x28 was 0x10000006, consistent with kind 6.
  `func_80006F8C_7B8C` collects kind-6 objects and inspects data+0x6C flags.
- `func_8000C768_D368` allocates a matrix with `func_8000671C_731C`, puts it at
  task+0x1C, selects task+0x2C data with +0x30 fallback, resolves model-related
  handles via `func_80005204_5E04`, and emits original graphics commands. The
  actor replacement should target this verified draw route, retaining the original
  scheduler, camera, collision and movement. The mesh hierarchy and display-list
  ownership still require tracing before substituting Kane.

The Kane element in Career Lab remains a textured reference-motion UI preview.
No gameplay replacement, collision integration or new Windows campaign package is
claimed by this research checkpoint.

## Experimental native geometry substitution

The authorized mod installer now adds a strong wrapper for the verified kind-6 emitter directly to the executable. It regenerates and calls the unmodified original function locally; original generated game code is excluded from delivery. `HH_ACTOR_DRAW_PROBE_PATH` optionally records bounded per-task draw commands and model fields. Task callbacks are phase dependent: the emitter writes its matrix pointer to task+0x1C.

`tools/build_kane_displaylists.py` compiles actual F3DEX2 libultra macros with the activated MIPS toolchain. The older Hybrid Heaven header rejects F3DEX2; use the retained Paper Mario gbi.h or provide `--gbi /absolute/path/PR/gbi.h`. Generated original textures, vertices and archives remain local under asset-import/kane-displaylists. The current archive contains 40 captured menu poses, 21 textured parts and 777 triangles per pose; it is not a walk or wrestling animation set.

`HH_KANE_IN_GAME=1` plus an absolute `HH_KANE_MODEL_PATH` enables an experimental replacement of data+0x28 for the gState+0xE0 candidate, only when geometry exists and the hierarchy flag is clear. The loader allocates its own librecomp mod heap, bounds the archive and relocations, converts guest byte order, and restores the original geometry pointer immediately after the original draw emitter returns. Original root transform, scheduler, collision and gameplay code execute. The material cache is invalidated around the substituted list. This is deliberately disabled by default. Hierarchical models, additional data+0x34 geometry, reset ownership, pose orientation/scale and scene render state still need live verification.

Validation: the native executable builds and contains the strong override; repeated installer execution avoids recompiling unchanged C/C++ files. `tests/kane_displaylist_test.py asset-import/kane-displaylists/kane-displaylists.bin` traverses all 40 compiled lists and validates 31,080 triangles, vertex cache indices, texture commands and 4,280 relocations. `tests/kane_model_test.cpp` verifies loader bounds, endian conversion, heap ownership and transient pointer restoration. These tests pass. The original New Game opening advances with live bounded draw observations; a controlled first-level Kane appearance and movement have not yet been verified. No new playable Windows campaign package is claimed.

Live integration found a real limitation missed by loader-only checks: RT64's normal segmented-address path truncates addresses above 16 MiB. The first player draw loaded the archive at 0x810000B0, then the unextended interpreter followed invalid display lists and crashed. Whole-frame extended addressing is unsuitable because original commands use addresses that depend on hardware DMA masking. The final implementation wraps only the model call in RT64's supported extension enable/address-mode/disable commands. It bounds expansion to the game's alternating display-list buffers (roots separated by 0x10048, as main_27C0 establishes), reserves tail space, and restores the original geometry call when capacity is insufficient. Tests verify the exact envelope, following commands, sentinels and capacity fallback.

The first-level E0 task uses an empty display list at 0x8017BC50 (DF000000), so it is the logical player root rather than the complete visible skin. Its 0x10008 flags compute a model matrix without submitting a matrix load. The replacement envelope therefore explicitly loads the computed task+0x1C matrix before Kane. Original visible descendant geometry still needs identification and suppression. The corrected native build and envelope tests pass; live first-level retest is pending.


## Controlled first-corridor checkpoint

The model-only extended-address envelope rendered Kane in the actual first
corridor. A one-second forward input changed the original player position and
the camera followed; this verifies original level movement with the imported
model. The initial result overlaps the original visible skin. Model scale 10
matches the root's 0.01 transform; the earlier scale 50 was too large.

The next build suppresses only draw commands from tasks whose bounded +0xC
parent chain reaches the active replaced root. Original draw functions still
execute so their matrices and scheduler effects remain intact. Parent linkage
was verified in original func_80005670. Synthetic ancestry, cycle, malformed
pointer, cursor restoration and display-list bounds checks pass. Live visual
confirmation of this hiding change is pending; menu reference poses still do
not constitute movement or wrestling animations. tools/run_native_kane.sh
provides the opt-in local launcher. No new Windows campaign package exists.

Windows cross-build preparation installed xwin 0.10.0. SDK downloads currently
receive proxy 403 for aka.ms and download.visualstudio.microsoft.com; these
precise destinations were added to the environment draft, which requires
review/save and publication. This draft does not change current connectivity.
Linux-cross-built DXIL is unsigned: a future Windows package must select Vulkan
as documented by the native project's BUILDING.md.


Reference capture now requires an explicitly named fresh output directory and
a description of the inspected motion, preserving the 40 earlier menu poses.
The parser ignores vertex loads before a force-matrix in its bounded preamble;
existing reference and malformed-data checks pass. One ring snapshot reconstructs
21 parts/777 triangles, but continuous in-ring capture remains blocked: stale
draw-list calls after an end marker can select an incomplete vertex cache.
Failed captures remain separate and are not promoted to animation archives.


Final live check of the ancestry change: Kane renders alone in the original
first corridor, with no visible Johnny overlap. A one-second W input moved
the player to (-300, 490.25, 241.386215), and the camera followed. The native
process continued without a renderer crash. The attached short corridor clip
shows captured idle reference poses, not a completed wrestling animation set.
Full combat, finishers and Windows packaging remain outstanding.
