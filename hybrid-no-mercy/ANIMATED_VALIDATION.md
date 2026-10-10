# Animated demo validation

- Windows cross-build passes with C++20, `-Wall -Wextra -Werror`, and static compiler runtime linking.
- Existing RPG engine suite: **115 checks passed**.
- Real 3D GUI under Wine: navigated the rooms and corridors, completed six sequential animated encounters using normal input, earned XP/credits, and spent earned points on training. No save editing or progress grants were used. Reopening restored the completed chapter and level-7 Kane.
- Fresh launch with no previous save creates a writable Kane career and can enter a career encounter. A separate launch imported the pre-existing cloud lab test save while retaining it.
- Practice: all six fighter finisher previews and a powerbomb complete. Career values remain identical before and after practice, with no item use or rewards.
- Earned credits bought Powerbomb and Kane's Chokeslam through the shop. The purchased finisher won an actual animated career battle and saved its rewards.
- Attire selections visibly change the original model colors and persist.
- Replacement Windows PCM player: opened a WAV, kept a loop buffer queued, stopped playback, and rejected a missing file. Captured output had a -2 dB peak. The final 3D app also produced captured output, with a -1.9 dB peak. Physical playback on the user's Windows PC remains for the user to verify.
- Native Hybrid Heaven Linux rendered build passes, including the added Career Lab tab. Its launcher rendered, a tab battle produced saved XP/level progression, and original textured boot graphics rendered. Full original-game gameplay and imported wrestling assets are not validated or implemented by this check.

Known limits: original rough procedural models/animations, three-room test facility, six career profiles, four original attire color variants, and two randomly selected entrance clips. This is not the full Hybrid Heaven/No Mercy game. Original models, exact attire assets, motion data, AKI combat integration, the original levels and the combined full campaign remain pending. Only career state resumes after closing; battle/world position does not.

Automated UI and audio check sources are included. Screenshots and the MP4 show the actual executable. No pre-progressed save, emulator save, complete ROM or native game assets are bundled.
