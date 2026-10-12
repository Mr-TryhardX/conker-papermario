#include "../integration/hh_battle_state.hpp"
#include <cassert>
#include <vector>
#include <cstdio>
int main(int argc,char** argv){
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
    const uint32_t actor=0x80202000,animation=0x80203000;
    write(actor,task);write(animation+0x1C,0x190015E);
    write(animation+0x20,0x00040100);write(animation+0x24,0x3F800000);
    hh_battle_state::State primary;
    assert(hh_battle_state::actor_animation(ram.data(),actor,animation,primary));
    assert(primary.resource==0x190015E && primary.frame()==4);
    assert(primary.resource!=state.resource && primary.frame()!=state.frame());
    assert(!hh_battle_state::actor_animation(ram.data(),0x807FFFFC,animation,primary));
    assert(!hh_battle_state::actor_animation(ram.data(),actor,0x807FFFDC,primary));
    auto bad_before=ram;
    hh_battle_state::Metadata meta;
    assert(!hh_battle_state::metadata(ram.data(),0,state,meta));
    assert(ram==bad_before);
    const uint32_t table=0x80300000,bank=0x80301000,clips=0x80302000,descriptor=0x80310000;
    write(table+state.bank()*4,bank);write(bank+4,clips);write(clips+state.clip()*4,descriptor);
    write(descriptor+8,0x00008020); // Original loop flag must not inflate length.
    assert(hh_battle_state::metadata(ram.data(),table,state,meta));
    assert(meta.address==descriptor && meta.first_frame==0 && meta.frame_count==32);
    write(clips+state.clip()*4,0x807FFFF4);assert(!hh_battle_state::metadata(ram.data(),table,state,meta));
    write(clips+state.clip()*4,descriptor);write(descriptor+8,0x8000);
    assert(!hh_battle_state::metadata(ram.data(),table,state,meta));
    unchanged=ram;
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
    if(argc==2){
        auto dir=std::filesystem::absolute(argv[1]);
        assert(!std::filesystem::exists(dir));std::filesystem::create_directory(dir);
#if defined(_WIN32)
        assert(_putenv_s("HH_BATTLE_SNAPSHOT_DIR",dir.string().c_str())==0);
#else
        assert(setenv("HH_BATTLE_SNAPSHOT_DIR",dir.string().c_str(),1)==0);
#endif
        write(task+0x2C,data);write(0x80000100,0x12345678);
        write(data+0x5C,0x01900001);write(data+0x60,0x00020100);
        auto existing=dir/"bank-0190-clip-0001-frame-0002.ram";
        {std::ofstream out(existing);out<<"preserve existing capture";}
        for(unsigned i=1;i<=10;++i){
            write(data+0x5C,0x01900000+i);auto same=ram;
            hh_battle_state::snapshot(ram.data(),task);assert(ram==same);
            hh_battle_state::snapshot(ram.data(),task);
        }
        assert(std::distance(std::filesystem::directory_iterator(dir),std::filesystem::directory_iterator{})==8);
        assert(std::filesystem::file_size(existing)==25);
        auto snapshot=dir/"bank-0190-clip-0002-frame-0002.ram";
        assert(std::filesystem::file_size(snapshot)==0x800000);
        std::ifstream in(snapshot,std::ios::binary);in.seekg(0x100);
        std::array<unsigned char,4> bytes{};in.read(reinterpret_cast<char*>(bytes.data()),4);
        assert((bytes==std::array<unsigned char,4>{0x12,0x34,0x56,0x78}));
        std::filesystem::remove_all(dir);
        std::puts("PASS: bounded guest-only snapshots, logical byte order, repeated-request suppression and existing-file preservation");
    }
    std::puts("PASS: battle-only animation decoding, root eligibility, bounds, duplicate suppression, cap and guest integrity");
}
