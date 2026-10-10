#pragma once
// Optional local research only. Reads guest memory; never changes the game.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hh_player_probe {
constexpr uint32_t base = 0x80000000u;
constexpr uint32_t size = 0x800000u;
struct Guest {
    const uint8_t* ram;
    bool span(uint32_t address, uint32_t bytes) const {
        return ram && address >= base && bytes <= size && address - base <= size - bytes && !(address & 3);
    }
    bool word(uint32_t address, uint32_t& value) const {
        if (!span(address, 4)) return false;
        std::memcpy(&value, ram + (address - base), 4);
        return true; // N64ModernRuntime stores host-endian words.
    }
    bool real(uint32_t address, float& value) const {
        uint32_t bits;
        if (!word(address, bits)) return false;
        std::memcpy(&value, &bits, 4);
        return std::isfinite(value);
    }
};
struct Player {
    uint32_t task = 0, data = 0, camera = 0;
    std::array<uint32_t, 12> task_words{}; // Unknown fields remain raw.
    std::array<float, 3> position{}, eye{}, target{};
};
inline bool read(const Guest& guest, Player& p) {
    Player next;
    if (!guest.word(0x801BBBF0u + 0xE0, next.task) ||
        !guest.span(next.task, 0x30) || !guest.word(next.task + 0x2C, next.data) ||
        !guest.span(next.data, 0x10)) return false;
    for (unsigned i = 0; i < next.task_words.size(); ++i)
        if (!guest.word(next.task + i * 4, next.task_words[i])) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!guest.real(next.data + 4 + i * 4, next.position[i])) return false;
    uint32_t camera_task;
    if (!guest.word(0x801BBCD8u, camera_task) ||
        !guest.span(camera_task, 0x30) || !guest.word(camera_task + 0x2C, next.camera) ||
        !guest.span(next.camera, 0x48)) return false;
    for (unsigned i = 0; i < 3; ++i)
        if (!guest.real(next.camera + 0x30 + i * 4, next.eye[i]) ||
            !guest.real(next.camera + 0x3C + i * 4, next.target[i])) return false;
    p = next;
    return true;
}
inline void tick(const uint8_t* ram) {
    static const char* path = std::getenv("HH_PLAYER_PROBE_PATH");
    if (!path || !*path) return;
    static uint64_t frame = 0;
    if (++frame % 30) return;
    Player p;
    if (!read({ram}, p)) return;
    // Append locally; this does not capture ROMs, textures or complete RAM.
    if (FILE* file = std::fopen(path, "a")) {
        std::fprintf(file, "{\"frame\":%llu,\"task\":%u,\"data\":%u,\"camera\":%u,\"position\":[%.9g,%.9g,%.9g],\"eye\":[%.9g,%.9g,%.9g],\"target\":[%.9g,%.9g,%.9g],\"task_words\":[",
            static_cast<unsigned long long>(frame), p.task, p.data, p.camera,
            p.position[0], p.position[1], p.position[2], p.eye[0], p.eye[1], p.eye[2], p.target[0], p.target[1], p.target[2]);
        for (unsigned i = 0; i < p.task_words.size(); ++i)
            std::fprintf(file, "%s%u", i ? "," : "", p.task_words[i]);
        std::fputs("]}\n", file);
        std::fclose(file);
    }
}
}
