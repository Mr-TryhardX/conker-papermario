#include "../integration/hh_battle_state.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
int main(){
    std::vector<uint8_t> ram(0x800000,0);
    auto write=[&](uint32_t a,uint32_t v){std::memcpy(ram.data()+a-0x80000000u,&v,4);};
    const uint32_t task=0x80200000,data=0x80201000;
    write(0x801BBCD0,task);write(task+0x2C,data);write(data+0x28,0x80170000);
    write(data+0x5C,0x01900169);write(data+0x60,0x00150100);write(data+0x64,0x3F800000);
    hh_battle_state::State state;
    assert(!hh_battle_state::read(ram.data(),task,state));
    ram[(0x1BBBF0u+0x29Au)^3u]=3;
    auto unchanged=ram;
    assert(hh_battle_state::read(ram.data(),task,state));
    assert(state.player && state.bank()==400 && state.clip()==361 && state.frame()==21);
    assert(state.frame_flags==0x00150100 && state.speed_bits==0x3F800000);
    hh_battle_state::Changes changes;
    assert(changes.accept(state));assert(!changes.accept(state));
    state.frame_flags=0x00160100;assert(changes.accept(state));
    state.player=false;assert(changes.accept(state));
    for(unsigned i=0;i<1000000;++i){state.frame_flags=i;changes.accept(state);}
    assert(changes.records==8192);
    assert(ram==unchanged);
    write(task+0xC,task+0x34);assert(!hh_battle_state::read(ram.data(),task,state));
    write(task+0xC,0);write(task+0x2C,0x807FFF9C);assert(!hh_battle_state::read(ram.data(),task,state));
    assert(!hh_battle_state::read(nullptr,task,state));
    assert(!hh_battle_state::read(ram.data(),0x7FFFFFFC,state));
    assert(!hh_battle_state::read(ram.data(),0x807FFFDC,state));
    std::puts("PASS: battle-only animation decoding, root eligibility, bounds, duplicate suppression, cap and guest integrity");
}
