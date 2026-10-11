# Experimental native Windows Kane demo

The local playable folder is `dist/NativeWindowsKane-Demo`; the ZIP is beside
it. Extract the whole ZIP, open its folder, then run **Play-Kane.bat**.
Use **Start Game** in the launcher. At the original title, press Enter, select
**New Game** with Space, then select **Game Start** with Space. Let the
opening movie finish: it still shows the original cast. Kane appears in the
first playable corridor. This is a Windows application.

Controls: WASD moves, Space is A, left Shift is B, R is the original R
button, Enter is Start and Escape opens native settings. E requests an
optional fixed-placement paired wrestling preview in field mode. That
preview does not apply damage or grab an original enemy.

The native x64 game and original first-corridor Kane movement were tested
under Wine 10 using Vulkan. All 115 career-engine checks passed as a
Windows executable, including save/backup/reload and corrupt-save rejection.
The 69-pose paired archive passed the Windows loader fixture; live Windows
E-button preview still needs testing. Physical Windows/GPU testing remains
user verification. Keep `native-launch.log` if launch fails. If Windows
reports a missing MSVCP/VCRUNTIME DLL, use the included official
`vc_redist.x64.exe` and launch again.

This remains a rough character-replacement demo. Complete original-level
and campaign playthrough, synchronized No Mercy combat/finishers, battle
music/cheers and full CAW integration are unfinished. Career Lab upgrades
and purchases are a separate prototype; they do not change original
campaign stats yet. Original music remains during gameplay. No Mercy music
can be auditioned in Career Lab. Automatic original saves are off; an
original save device/slot and save/load cycle still need validation.

The ZIP includes supplied local game data and imported model archives. It
is kept local; public delivery Git contains authored code and bounded
validation evidence. Earlier prototypes and original saves are preserved.

Rebuild in the prepared cloud:

```sh
bash tools/prepare_windows_toolchain.sh
python3 tools/apply_native_integration.py
bash tools/configure_native_windows.sh
cmake --build native/HybridHeaven/build-win-ready -j 4
```

Use the main runtime Python when tools need NumPy; activation selects a
different virtual environment. The installer needs the retained supplied
ROM-derived generated sources. See MOTION_CHECKPOINT.md for research state.
