#pragma once
// Read-only animation state observed in the original kind-6 draw path.
// 8000E5D4 splits the resource key into high bank / low clip; 8000E748
// reads the frame halfword at state+4 and speed float at state+8.
// Move names and attacker/defender roles are deliberately not inferred here.
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hh_battle_state {
inline bool span(uint32_t a,uint32_t n){return a>=0x80000000u && n<=0x800000u && a-0x80000000u<=0x800000u-n && !(a&3u);}
inline uint32_t word(const uint8_t* ram,uint32_t a){uint32_t v;std::memcpy(&v,ram+a-0x80000000u,4);return v;}
struct State {
    uint32_t task=0,data=0,resource=0,frame_flags=0,blend_bits=0,speed_bits=0;
    bool player=false;
    unsigned bank()const{return resource>>16;}
    unsigned clip()const{return resource&65535u;}
    unsigned frame()const{return frame_flags>>16;}
};
inline bool read(const uint8_t* ram,uint32_t task,State& result){
    if(!ram || ram[(0x1BBBF0u+0x29Au)^3u]!=3 || !span(task,0x30) || word(ram,task+0xC))return false;
    uint32_t data=word(ram,task+0x2C);
    if(!span(data,0x68) || !word(ram,data+0x28) || (word(ram,data+0x24)&0x80000000u))return false;
    State s;
    s.task=task;s.data=data;s.player=word(ram,0x801BBCD0u)==task;
    s.resource=word(ram,data+0x5C);s.frame_flags=word(ram,data+0x60);
    s.blend_bits=word(ram,data+0x50);s.speed_bits=word(ram,data+0x64);
    result=s;return true;
}
struct Changes {
    std::array<State,16> previous{};
    unsigned records=0;
    bool accept(const State& s){
        if(records>=8192)return false;
        for(auto& p:previous){
            if(p.task && p.task!=s.task)continue;
            bool changed=!p.task || p.data!=s.data || p.resource!=s.resource || p.frame_flags!=s.frame_flags || p.blend_bits!=s.blend_bits || p.speed_bits!=s.speed_bits || p.player!=s.player;
            p=s;if(changed)++records;return changed;
        }
        return false;
    }
};
inline void observe(const uint8_t* ram,uint32_t task){
    static const char* path=std::getenv("HH_BATTLE_ANIMATION_PROBE_PATH");
    if(!path || !*path)return;
    static Changes changes;
    State s;if(!read(ram,task,s) || !changes.accept(s))return;
    const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if(FILE* f=std::fopen(path,"a")){
        std::fprintf(f,"{\"record\":%u,\"host_ms\":%lld,\"task\":%u,\"data\":%u,\"player\":%s,\"bank\":%u,\"clip\":%u,\"frame\":%u,\"frame_flags_raw\":%u,\"blend_bits_raw\":%u,\"speed_bits_raw\":%u}\n",changes.records,static_cast<long long>(now),s.task,s.data,s.player?"true":"false",s.bank(),s.clip(),s.frame(),s.frame_flags,s.blend_bits,s.speed_bits);
        std::fclose(f);
    }
}
}
