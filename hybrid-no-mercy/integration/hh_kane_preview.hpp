#pragma once
#include <cstdint>
#include <cstddef>
#include <optional>

// Explicit renderer research only. Does not select a game move or apply damage.
// A changed nonzero request starts one preview; zero cancels it. Keeping the
// request unchanged cannot restart the animation on repeated draw callbacks.
struct KanePreview {
    uint32_t task=0,request=0;
    int64_t started=0;
    bool active=false;
    static constexpr int64_t frame_ms=50;
    std::optional<size_t> sample(uint32_t next_task,uint32_t next_request,
                                int64_t now,size_t frames,bool allowed=true) {
        if(task!=next_task) {
            task=next_task;request=next_request;active=false;return {};
        }
        if(!allowed) {request=next_request;active=false;return {};}
        if(request!=next_request) {
            request=next_request;started=now;active=request!=0;
        }
        if(!frames) {active=false;return {};}
        if(!active)return {};
        if(now<started || (now-started)/frame_ms>=static_cast<int64_t>(frames)) {
            active=false;return {};
        }
        return static_cast<size_t>((now-started)/frame_ms);
    }
};

// Optional research control on the game's unused L button. A held button may
// never become a delayed request after a scene change or disallowed mode.
struct KanePreviewButton {
    uint32_t task=0, counter=0;
    bool down=false;
    std::optional<uint32_t> sample(uint32_t next_task,bool next_down,bool allowed) {
        if(task!=next_task) {task=next_task;down=next_down;return {};}
        bool edge=next_down && !down;
        down=next_down;
        if(!allowed || !edge)return {};
        if(++counter==0)counter=1;
        return counter;
    }
};
