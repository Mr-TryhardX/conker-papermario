# Facility Breach — animated world prototype

Extract the **whole ZIP** and double-click `AnimatedDemo.exe`. Keep the `assets` folder beside it. This is a Windows application, not a Project64 ROM. It requires a working OpenGL graphics driver. A 1280 × 900 or larger desktop is recommended; the game window can be resized.

This build adds an original 3D test facility, rough articulated wrestler models, walking and combat animations to the existing career rules. It is **not the full Hybrid Heaven/No Mercy crossover**: the original games' models, costumes, maps, wrestling engine, and animations have not been imported.

## See a powerbomb right away

Press **P** to enter practice, then **7** for the powerbomb and **8** for the displayed fighter's finisher. **Tab** cycles through all six preview fighters, including those still locked in your career. Every move is available in practice, and practice does not spend items or award progress. Wait for an animation to finish before choosing another move. **Esc** returns to exploration.

The original rough finisher animations are Chokeslam for Kane, Tombstone for Undertaker, Stunner for Stone Cold, Rock Bottom, Angle Slam, and a spinning kick for Johnny. These are hand-built procedural approximations, not No Mercy's imported motion data.

## Play the chapter

- **WASD:** move; **Shift:** run; **left/right arrows:** turn the exploration camera.
- **E:** interact with a nearby red or green encounter circle. Complete encounters in order; green circles can be replayed for reduced rewards.
- **1–8 in a battle:** Jab, Kick, Body Slam, Suplex, DDT, Submission, Powerbomb, and your fighter's finisher. Career moves must be learned in the shop first.
- **G:** guard and regain stamina; **R:** recover more stamina; **H:** use a medkit.
- **Esc:** close a menu or retreat from a battle; animations must finish first.
- **F1:** help; **F2:** roster and attire; **F3:** training; **F4:** shop; **F5:** records.
- **F10:** return to the lab entrance when outside combat.
- **M:** toggle music; **F12:** save a screenshot and diagnostic snapshot beside the executable.

The chapter consists of three rooms connected by corridors, with six encounters. Win battles to earn XP and credits. Each level grants three training points. Kane's Grapple perk helps with throws; upgrade Grapple in Training, then purchase new moves in the shop. Powerbomb unlocks for purchase at level 4; the finisher unlocks at level 5. Only practice bypasses these requirements.

Each career profile retains its own level, stats, moves, and attire. The wallet and medkits are shared. Attire now visibly changes the rough model's colors; the four variants are original prototype designs, **not all the real No Mercy costumes**.

## Progress and music

Progress saves in `%LOCALAPPDATA%\HybridNoMercyAnimatedDemo\career.json`. On the first run, a valid existing `%LOCALAPPDATA%\HybridNoMercyCareerLab\career.json` is copied into the new career automatically. The original lab save stays intact. Later changes in the two applications are separate. Individual fight state and world position do not resume on reopening; career progress does.

No Mercy shop music plays in exploration and menus. Each battle randomly selects a Kane or Rock entrance clip; the clips loop and repeat choices are possible. The replacement player owns a Windows audio output buffer and reports playback errors. If the files play in a media player but the app is silent, check Windows **Settings → System → Sound → Volume mixer** for this executable, its volume and output device. `audio-status.txt` beside the executable records file presence and any playback error.

## Source

`bash tools/build_animated.sh` runs the career tests and cross-compiles the Windows application using GCC/MinGW. `tests/audio_smoke.cpp` checks the actual Windows WAV player with an available audio output device. `tools/check_animated_ui.py` drives a fresh or incomplete cloud test career through the real 3D UI under the local X11/Wine session; it never grants progress by editing saves.

Original prototype code is GPL-3.0-or-later; see COPYING and THIRD_PARTY_NOTICES.md. Music clips were captured from the user-supplied ROM. No full ROM or pre-progressed save is included.
