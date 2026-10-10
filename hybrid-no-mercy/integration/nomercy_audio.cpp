#include "nomercy_audio.h"
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <vector>
namespace {
struct Audio {
    SDL_AudioDeviceID device=0;
    std::atomic<uint64_t> heartbeat{0};
    bool tried=false;
    std::vector<int16_t> music,sfx;
    size_t music_pos=0,sfx_pos=0;
    ~Audio(){if(device)SDL_CloseAudioDevice(device);}
    static void callback(void* userdata,Uint8* output,int bytes){
        auto& a=*static_cast<Audio*>(userdata);memset(output,0,bytes);
        if(SDL_GetTicks64()-a.heartbeat.load(std::memory_order_relaxed)>300)return;
        auto* dst=reinterpret_cast<int16_t*>(output);
        for(int i=0;i<bytes/2;++i){
            int value=0;
            if(!a.music.empty()){value=int(a.music[a.music_pos++])*45/100;a.music_pos%=a.music.size();}
            if(a.sfx_pos<a.sfx.size())value+=int(a.sfx[a.sfx_pos++])*65/100;
            dst[i]=int16_t(std::clamp(value,-32768,32767));
        }
    }
    void init(){
        if(tried)return;tried=true;
        SDL_AudioSpec want{};want.freq=44100;want.format=AUDIO_S16SYS;want.channels=2;want.samples=1024;want.callback=callback;want.userdata=this;
        device=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);
        if(!device){fprintf(stderr,"[nomercy-audio] output error: %s\n",SDL_GetError());return;}
        set(false,"assets/nomercy/audio/shop.wav");SDL_PauseAudioDevice(device,0);
    }
    void set(bool effect,const char* path){
        if(!device)return;
        SDL_AudioSpec spec{};Uint8* data=nullptr;Uint32 bytes=0;
        if(!SDL_LoadWAV(path,&spec,&data,&bytes)){fprintf(stderr,"[nomercy-audio] cannot load %s: %s\n",path,SDL_GetError());return;}
        SDL_AudioCVT cvt{};const int result=SDL_BuildAudioCVT(&cvt,spec.format,spec.channels,spec.freq,AUDIO_S16SYS,2,44100);
        if(result<0||bytes>64*1024*1024){SDL_FreeWAV(data);return;}
        std::vector<Uint8> converted(size_t(bytes)*std::max(1,cvt.len_mult));memcpy(converted.data(),data,bytes);SDL_FreeWAV(data);
        cvt.buf=converted.data();cvt.len=int(bytes);
        if(result&&SDL_ConvertAudio(&cvt)<0){fprintf(stderr,"[nomercy-audio] conversion error: %s\n",SDL_GetError());return;}
        const size_t length=result?size_t(cvt.len_cvt):bytes;
        std::vector<int16_t> pcm(length/2);memcpy(pcm.data(),converted.data(),pcm.size()*2);
        SDL_LockAudioDevice(device);
        if(effect){sfx=std::move(pcm);sfx_pos=0;}else{music=std::move(pcm);music_pos=0;}
        SDL_UnlockAudioDevice(device);
        fprintf(stderr,"[nomercy-audio] %s loaded %s (%zu PCM samples)\n",effect?"sample":"music",path,length/2);
    }
} audio;
}
void recompui::nomercy_audio_heartbeat(){audio.heartbeat.store(SDL_GetTicks64(),std::memory_order_relaxed);audio.init();}
void recompui::nomercy_audio_action(int index){
    audio.init();if(!audio.device)return;
    if(index==0)audio.set(false,"assets/nomercy/audio/shop.wav");
    else if(index==1)audio.set(false,"assets/nomercy/audio/Kane.wav");
    else if(index==2)audio.set(false,"assets/nomercy/audio/The-Rock.wav");
    else if(index==3){SDL_LockAudioDevice(audio.device);audio.music.clear();audio.sfx.clear();SDL_UnlockAudioDevice(audio.device);}
    else if(index>=100&&index<265){char path[160];snprintf(path,sizeof(path),"assets/nomercy/audio/bank1/%03d.wav",index-100);audio.set(true,path);}
}
