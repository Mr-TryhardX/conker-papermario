#include "../integration/hh_kane_preview.hpp"
#include <cassert>
#include <cstdio>
int main() {
    KanePreview p;
    assert(!p.sample(1,0,1000,69));
    assert(p.sample(1,1,1100,69)==0);
    assert(p.sample(1,1,1100,69)==0); // repeated draw does not advance
    assert(p.sample(1,1,1150,69)==1);
    assert(p.sample(1,1,4500,69)==68);
    assert(!p.sample(1,1,4550,69)); // finish once, without looping
    assert(!p.sample(1,1,5000,69));
    assert(p.sample(1,2,5100,69)==0);
    assert(!p.sample(2,2,5200,69)); // room/actor rebind cancels
    assert(!p.sample(2,2,5300,69)); // old request cannot retrigger
    assert(p.sample(2,3,5400,69)==0);
    assert(!p.sample(2,0,5450,69)); // explicit cancel
    assert(p.sample(2,4,5500,69)==0);
    assert(!p.sample(2,4,5490,69)); // backward clock cancels
    assert(!p.sample(2,5,5600,0));
    assert(!p.sample(2,5,5700,69)); // missing archive cannot leave a latent action
    std::puts("Kane preview: timed frames, duplicate draws, completion, cancellation and actor rebinding passed.");
}
