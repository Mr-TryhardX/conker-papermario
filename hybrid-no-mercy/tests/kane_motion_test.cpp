#include "../integration/hh_kane_motion.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main() {
    KaneMotion m;
    assert(!m.sample(1,2,10,10,1000));
    assert(!m.sample(1,2,10.001f,10,1030));
    assert(m.sample(1,2,12,10,1060));
    assert(m.frame(40)==9); // displacement, independent of submitted draw count
    auto phase=m.frame(40);
    assert(m.sample(1,2,12,10,1060) && m.frame(40)==phase);
    assert(m.sample(1,2,12,10,1150));
    assert(!m.sample(1,2,12,10,1185)); // stopped against a wall
    assert(m.sample(1,2,12,14,1200));
    assert(!m.sample(1,2,100,100,1230) && m.frame(40)==0); // teleport
    assert(!m.sample(3,4,100,100,1260)); // room/player rebind
    assert(!m.sample(3,4,101,101,2500)); // stale interval
    assert(!m.sample(3,4,std::numeric_limits<float>::quiet_NaN(),101,2530));
    assert(!m.sample(3,4,101,101,2560));
    assert(m.sample(3,4,101,103,2590));
    assert(!m.sample(3,4,101,103,2000)); // clock reset
    assert(m.frame(0)==0);
    std::puts("Kane motion: displacement phase, draw duplication, collision stops, teleports and invalid coordinates passed.");
}
