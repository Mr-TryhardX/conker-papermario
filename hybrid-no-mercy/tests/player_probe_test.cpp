#include "../integration/hh_player_probe.hpp"
#include <cassert>
#include <limits>
#include <vector>
int main() {
    using namespace hh_player_probe;
    std::vector<uint8_t> ram(size);
    Guest guest{ram.data()};
    auto put = [&](uint32_t address, uint32_t value) { std::memcpy(ram.data() + address - base, &value, 4); };
    auto real = [&](uint32_t address, float value) { uint32_t bits; std::memcpy(&bits, &value, 4); put(address, bits); };
    uint32_t word = 99;
    assert(!guest.word(base - 4, word) && word == 99);
    assert(!guest.word(base + size, word));
    assert(!guest.word(base + 1, word));
    assert(guest.word(base + size - 4, word));
    uint8_t byte=99;
    assert(!guest.byte(base-1,byte) && byte==99);
    assert(!guest.byte(base+size,byte));
    ram[0x1BBE8A^3]=3;
    ram[0x1BBE44^3]=2;ram[0x1BBE45^3]=8;ram[0x1BBE46^3]=1;
    assert(guest.byte(0x801BBE8A,byte) && byte==3);
    Player p;
    assert(!read(guest, p));
    put(0x801BBCD0, 0x80200000);
    put(0x8020002C, 0x80201000);
    put(0x801BBCD8, 0x80202000);
    put(0x8020202C, 0x80203000);
    real(0x80201004, 12.5f); real(0x80201008, -7.0f); real(0x8020100C, 9.0f);
    assert(read(guest, p));
    assert(p.task == 0x80200000 && p.position[0] == 12.5f && p.position[1] == -7.0f);
    assert(p.camera_mode==3 && (p.scene==std::array<uint8_t,3>{2,8,1}));
    auto original = ram;
    assert(read(guest, p) && ram == original);
    real(0x80201004, std::numeric_limits<float>::infinity());
    assert(!read(guest, p) && p.position[0] == 12.5f);
    put(0x8020002C, base - 4);
    assert(!read(guest, p)); // reject a data pointer before RAM, even if its fields land inside
    put(0x8020002C, 0xFFFFFFFC);
    assert(!read(guest, p)); // wrapped offsets do not escape guest bounds
    std::puts("Player probe: bounds, pointer chain, finite coordinates and read-only checks passed.");
}
