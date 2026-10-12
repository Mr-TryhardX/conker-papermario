#pragma once
#include <cstdint>
namespace recompui {
void nomercy_audio_heartbeat();
void nomercy_audio_action(int index);
// Opt-in host-side soundtrack. Reads the observed camera mode; no guest writes.
void nomercy_battle_audio_tick(const uint8_t* ram, int main_volume=100);
int nomercy_original_audio_percent();
}
