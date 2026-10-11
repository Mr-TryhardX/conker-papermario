The current goal is the full Hybrid Heaven campaign with No Mercy assets. Kane now replaces the player model in the native first corridor, with movement-driven walking and an optional decoded one-shot animation preview. The original first door, NPC dialogue and Code Key pickup have been exercised. An experimental native Windows package now boots the original game and renders Kane moving through the first corridor under Wine. Paired combat and complete-level validation remain unfinished. See [Windows demo](NATIVE_WINDOWS_DEMO.md). See [motion checkpoint](MOTION_CHECKPOINT.md) and [current scope](PROJECT_SCOPE.md).

# Hybrid / No Mercy — Career Systems Lab

The newer **Animated World Prototype** adds a playable 3D facility and visible attacks and finishers. See [ANIMATED_DEMO.md](ANIMATED_DEMO.md) and run `AnimatedDemo.exe` from that download. The original menu-only lab remains available below.

This first Windows prototype lets you test a wrestling RPG career loop. It is a separate menu-driven application, not a merged N64 ROM. There is no 3D world or animated wrestling in this build. It does not run in Project64.

## Play

Extract **the entire ZIP** into a folder, then double-click `CareerLab.exe`. Keep its `assets` folder beside it for music. Use the mouse, or arrow keys and Enter/Space. A display at least 1280 × 900 is recommended.

New careers start as Kane. Open **Career**, select **Training Floor**, and choose moves to defeat the opponent. Try Body Slam. Winning earns credits and XP; leveling up gives three training points. Spend these in **Training**, buy moves or medkits in **Shop**, and select the next unlocked encounter in **Career**. Guard reduces damage and restores stamina; Recover restores more stamina. Medkits heal during combat. Retreat returns to the menus.

## Included

- Six sequential original test encounters, with reduced rewards for replaying cleared fights.
- Kane, Stone Cold, The Rock, Kurt Angle, The Undertaker, and Johnny Slater profiles with individual stats and perks. Rock, Angle, and Undertaker cost credits to unlock.
- Separate levels, XP, training, and purchased moves for each profile; a shared wallet and medkit supply.
- Four remembered attire slots per profile. **These are selection slots only: no costume models or textures are displayed yet.**
- Automatic save/load, backup save, chapter records, and an in-app button to open the save folder.
- No Mercy shop music in menus. Each battle randomly chooses between the supplied Kane and Rock entrance clips. Clips loop; a repeat choice is possible.

## Save files

Career progress lives in `%LOCALAPPDATA%\HybridNoMercyCareerLab\career.json`, with the previous save in `career.json.bak`. This is separate from emulator and original game saves. Individual fight state is not restored: reopening returns to the menus, retaining career changes and consumed items. An unreadable existing save blocks changes rather than silently replacing it. Back it up before repairing or starting over.

## What remains

Hybrid Heaven world integration, imported No Mercy wrestler models and real costumes, move animations, AKI wrestling combat, exact original menu artwork, the full roster, additional music, and an actual combined campaign are still pending. This prototype is **not 25% of that full game**. Its simplified turn-based battles validate RPG rules and save behavior.

## Development

`bash tools/build_lab.sh` runs 115 career-engine checks and cross-compiles the Windows application. It needs Linux GCC with C++20 and `x86_64-w64-mingw32-g++-posix`; `MINGW_CXX` can override the latter. The JSON header is bundled with its MIT notice.

The `integration` files and `tools/apply_native_integration.py` add a separate Career Lab settings tab to the local Hybrid Heaven recompilation. The Linux native rendered build now passes; its launcher, a career battle, and original game's boot graphics were checked. This tab remains separate from original Hybrid Heaven gameplay. It is not part of the downloadable Windows prototype.

Music was captured from the user-supplied No Mercy ROM. No complete ROM is included. Original music remains owned by its respective rights holders. Original career code is GPL-3.0-or-later; see `COPYING` and `THIRD_PARTY_NOTICES.md`.
