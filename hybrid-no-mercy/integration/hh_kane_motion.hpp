#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>

// Drive captured locomotion from actual displacement, including collision
// stops. No controller input or original movement state is modified.
struct KaneMotion {
    // Initial eight-unit cycle repeated about twelve times per second at the
    // observed corridor speed. Eighty units gives a usable starting cadence.
    static constexpr double cycle_distance=80.0;
    uint32_t task=0, data=0;
    float x=0, z=0;
    double distance=0;
    int64_t last_ms=0, moving_until=0;
    bool valid=false;
    bool sample(uint32_t next_task,uint32_t next_data,float next_x,float next_z,int64_t now) {
        if(!std::isfinite(next_x) || !std::isfinite(next_z)) {
            valid=false;distance=0;moving_until=0;return false;
        }
        if(!valid || task!=next_task || data!=next_data || now<last_ms || now-last_ms>1000) {
            task=next_task;data=next_data;x=next_x;z=next_z;last_ms=now;
            valid=true;distance=0;moving_until=0;return false;
        }
        double step=std::hypot(double(next_x)-x,double(next_z)-z);
        x=next_x;z=next_z;last_ms=now;
        if(step>25) { distance=0;moving_until=0;return false; }
        if(step>.005) {
            distance=std::fmod(distance+step,cycle_distance);
            moving_until=now+125;
        }
        return now<moving_until;
    }
    size_t frame(size_t count) const {
        return count ? static_cast<size_t>(distance*count/cycle_distance)%count : 0;
    }
};
