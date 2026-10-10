# Read-only No Mercy runtime research

`research_controller.c` is a local Mupen64Plus 2.6 input plugin. It supplies ordinary button/stick state from `MERCY_RESEARCH_INPUT` and can capture only the core's guest RDRAM through `DebugMemGetPointer(1)` when `MERCY_CAPTURE_RAM_FLAG` exists, writing `MERCY_CAPTURE_RAM_PATH`. It does not read host memory or edit game state. Compile using the bundled `m64p_plugin.h`/`m64p_common.h` headers with `-O2 -Wall -Wextra -Werror -fPIC -shared -ldl`; build a new output and stop the research emulator before replacing a loaded plugin. Keep snapshots and original game saves local.

The API version is 0x020100, required by this core. The SDL swap preload used earlier stalled rendering; use the controller debugger path instead. Ordinary input and a complete 8 MiB guest snapshot were verified in the local emulator. `research_input.py` atomically writes inputs and returns to neutral after each press.

Verified menu path after boot: Start; L selects Multi Play; A enters; A selects Exhibition; Start accepts Single Match. On Superstars 1, Down then Right highlights Kane; A confirms player 1. In the local reference both players were Kane, showing different attire variants. Kane's full original 3D actor rendered in No Mercy. This verifies a reference only, not an imported actor in Hybrid Heaven.

The resident ROM/VMA relation is ROM = VMA - 0x80000000 + 0xC00. The resident renderer includes func_800032A4 -> func_80002FA4; the draw-object pointer table is 0x80061D40. Standard F3DEX2 display lists were observed. Original compact mesh byte prefixes were not found directly in the rendered actor snapshot; conversion/loading must be resolved rather than assuming extracted meshes are laid out verbatim in runtime RAM. A frame streaming routine around func_80011340 uses 98-byte records; semantic skeletal decoding remains pending.

Hybrid Heaven upstream camera research documents D_801BBBF0 + 0xE0 as player task pointer, task + 0x2C as data, world XYZ at +4/+8/+C, facing at +0x12. These are research leads; no player RAM modification or speculative actor injection is implemented.
