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
    assert(p.sample(2,6,5800,69)==0);
    assert(!p.sample(2,6,5850,69,false)); // entering a cutscene cancels immediately
    assert(!p.sample(2,6,5900,69,true)); // returning to field cannot resume
    assert(!p.sample(2,7,5950,69,false)); // disallowed request is consumed
    assert(!p.sample(2,7,6000,69,true));
    assert(p.sample(2,8,6050,69,true)==0);
    std::puts("Kane preview: timed frames, duplicate draws, completion, cancellation and actor rebinding passed.");
    KanePreviewButton button;
    assert(!button.sample(10,false,true));
    assert(button.sample(10,true,true)==1);
    assert(!button.sample(10,true,true));
    assert(!button.sample(10,false,true));
    assert(!button.sample(10,true,false));
    assert(!button.sample(10,true,true)); // no deferred cutscene press
    assert(!button.sample(20,true,true)); // no held press after rebinding
    assert(!button.sample(20,false,true));
    assert(button.sample(20,true,true)==2);
    button.counter=UINT32_MAX;
    assert(!button.sample(20,false,true));
    assert(button.sample(20,true,true)==1); // wrap never requests cancellation
    std::puts("Preview button: edges, holds, disallowed modes, rebinding and counter wrap passed.");
}
