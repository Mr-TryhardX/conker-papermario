#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

// Sample an imported clip from the original animation cursor, never wall time.
// An unbound action, missing duration or non-battle mode immediately falls back.
inline std::optional<size_t> kane_combat_frame(unsigned camera_mode,
        uint32_t resource,uint32_t binding,unsigned cursor,unsigned original_frames,
        size_t imported_frames){
    if(camera_mode!=3 || !binding || resource!=binding || !original_frames ||
       !imported_frames || imported_frames>120)return {};
    if(original_frames==1 || imported_frames==1)return 0;
    uint64_t position=std::min(cursor,original_frames-1);
    return size_t(position*(imported_frames-1)/(original_frames-1));
}
