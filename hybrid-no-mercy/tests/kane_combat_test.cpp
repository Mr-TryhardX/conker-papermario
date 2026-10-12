#include "../integration/hh_kane_combat.hpp"
#include <cassert>
#include <cstdio>
int main(){
    constexpr uint32_t key=0x190015C;
    assert(!kane_combat_frame(2,key,key,0,20,9));
    assert(!kane_combat_frame(3,key,0,0,20,9));
    assert(!kane_combat_frame(3,key+1,key,0,20,9));
    assert(!kane_combat_frame(3,key,key,0,0,9));
    assert(!kane_combat_frame(3,key,key,0,20,0));
    assert(!kane_combat_frame(3,key,key,0,20,121));
    assert(kane_combat_frame(3,key,key,0,20,9)==0);
    assert(kane_combat_frame(3,key,key,19,20,9)==8);
    assert(kane_combat_frame(3,key,key,65535,20,9)==8);
    assert(kane_combat_frame(3,key,key,0,1,9)==0);
    assert(kane_combat_frame(3,key,key,10,20,1)==0);
    size_t last=0;
    for(unsigned i=0;i<20;++i){auto f=kane_combat_frame(3,key,key,i,20,9);assert(f && *f>=last && *f<9);last=*f;}
    // Entering a clip midway samples the correct phase; no delayed timer/replay.
    assert(kane_combat_frame(3,key,key,10,20,9)==4);
    assert(!kane_combat_frame(0,key,key,10,20,9));
    assert(kane_combat_frame(3,key,key,0,20,9)==0);
    std::puts("PASS: original-cursor combat sampling, endpoints, monotonicity, immediate cancellation and malformed inputs");
}
