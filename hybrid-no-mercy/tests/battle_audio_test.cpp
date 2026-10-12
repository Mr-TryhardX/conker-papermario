// Real SDL dummy-output fixture; WAV decoding and device startup are exercised.
#include "../integration/nomercy_audio.h"
#include <SDL.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>

static void wav(const std::filesystem::path& path){
    std::ofstream f(path,std::ios::binary);
    auto u16=[&](unsigned v){f.put(char(v));f.put(char(v>>8));};
    auto u32=[&](unsigned v){u16(v);u16(v>>16);};
    f.write("RIFF",4);u32(36+44100*4);f.write("WAVEfmt ",8);u32(16);
    u16(1);u16(2);u32(44100);u32(44100*4);u16(4);u16(16);
    f.write("data",4);u32(44100*4);
    for(unsigned i=0;i<44100;++i){u16((i%100<50)?4000:unsigned(-4000)&65535);u16((i%100<50)?4000:unsigned(-4000)&65535);}
    assert(f.good());
}
int main(int argc,char** argv){
    assert(argc==3);
    const bool enabled=std::strcmp(argv[2],"enabled")==0;
    auto old=std::filesystem::current_path();
    auto dir=std::filesystem::absolute(argv[1]);
    assert(!std::filesystem::exists(dir));
    std::filesystem::create_directories(dir/"assets/nomercy/audio");
    wav(dir/"assets/nomercy/audio/Kane.wav");wav(dir/"assets/nomercy/audio/The-Rock.wav");
    std::filesystem::current_path(dir);
    SDL_SetMainReady();
    assert(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER)==0);
    std::vector<uint8_t> ram(0x800000,0);
    auto mode=[&](uint8_t value){ram[(0x1BBBF0u+0x29Au)^3u]=value;};
    mode(2);recompui::nomercy_battle_audio_tick(ram.data());
    assert(recompui::nomercy_original_audio_percent()==100);
    mode(3);auto original=ram;
    recompui::nomercy_battle_audio_tick(ram.data());
    assert(ram==original);
    assert(recompui::nomercy_original_audio_percent()==(enabled?15:100));
    for(unsigned i=0;i<6;++i){SDL_Delay(60);recompui::nomercy_battle_audio_tick(ram.data());}
    assert(recompui::nomercy_original_audio_percent()==(enabled?15:100));
    SDL_Delay(350);assert(recompui::nomercy_original_audio_percent()==100);
    recompui::nomercy_battle_audio_tick(ram.data());
    assert(recompui::nomercy_original_audio_percent()==(enabled?15:100));
    mode(2);recompui::nomercy_battle_audio_tick(ram.data());
    assert(recompui::nomercy_original_audio_percent()==100);
    std::filesystem::remove_all("assets");
    mode(3);recompui::nomercy_battle_audio_tick(ram.data());
    assert(recompui::nomercy_original_audio_percent()==100); // Missing music must not silence game.
    recompui::nomercy_battle_audio_tick(nullptr);
    assert(recompui::nomercy_original_audio_percent()==100);
    std::filesystem::current_path(old);std::filesystem::remove_all(dir);
    std::puts("PASS: soundtrack transition, timeout, guest integrity, missing-file fallback and opt-in behavior");
}
