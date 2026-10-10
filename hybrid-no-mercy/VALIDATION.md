# Validation — first career prototype

Verified in the cloud environment:

- Career engine: **115 checks passed**, including completion of all six encounters, rewards, level thresholds, stat upgrades, purchases, character unlocks, costume slot persistence, individual perks, defeat/retreat, corrupt-save rejection, legacy save migration, and durable save/load.
- Windows build: MinGW C++20 compilation with `-Wall -Wextra -Werror`, statically linked standard runtime.
- Windows GUI under Wine: selected Kane and attire slot 4; completed Training Floor with Body Slam; reached level 2; upgraded Power; bought Suplex; closed and reopened the application and verified those values were restored.
- Music: menu shop clip included; separate battle starts selected both Kane and Rock clips. PCM WAV clips were recorded from the supplied No Mercy ROM. Host playback through an actual Windows PC sound device has not been tested.
- Hybrid Heaven original decompression and N64/RSP code generation passed. Native headless binary built and executed the game entry point, creating a window and opening audio. This headless check cannot validate rendered graphics.
- Experimental native Career Lab tab: source syntax check passed. Full rendered build remains unverified: GTK dependency discovery currently lacks egl/gl/cloudproviders pkg-config prerequisites. It is not part of the downloadable Windows app.

Not implemented: merged 3D world, imported wrestler models, real attire assets, wrestling animation/combat integration, full roster, or combined story campaign. Attire slots are data selections only.

Screenshots document the actual prototype UI. Download contains no emulator save or pre-progressed career; a new Windows user starts with a fresh Kane career.
