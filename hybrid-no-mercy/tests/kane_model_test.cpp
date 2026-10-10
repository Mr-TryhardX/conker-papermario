#include "../integration/hh_kane_model.cpp"
#include <cassert>
#include <filesystem>
#include <limits>

namespace recomp {
void* alloc(uint8_t* ram,size_t size) { assert(size<0x200000); return ram+0x1000100; }
void free(uint8_t*,void*) { assert(false && "Unexpected invalid allocation"); }
}
int main(int argc,char** argv) {
    assert(argc==2);
    setenv("HH_KANE_MODEL_PATH",argv[1],1);
    setenv("HH_KANE_IN_GAME","1",1);
    std::vector<uint8_t> ram(0x1400000,0xA5);
    Model fixture;
    fixture.load(ram.data());
    assert(fixture.frames.size()==40 && fixture.address==0x81000100);
    assert(ram[0x10000FF]==0xA5 && ram[0x1200100]==0xA5);
    for(auto frame:fixture.frames) {
        size_t o=frame-0x80000000u;
        uint32_t op; std::memcpy(&op,ram.data()+o,4);
        assert(op==0xE7000000); // correctly converted guest byte/word order
    }
    uint32_t task=0x80200000,data=0x80201000,original=0;
    write(ram.data(),0x801BBCD0,task);
    write(ram.data(),task+0x2C,data);
    write(ram.data(),data+0x24,0x10008);
    write(ram.data(),data+0x28,0x8017BC50);
    assert(!hh_kane_override_begin(ram.data(),task+4,&original));
    write(ram.data(),data+0x24,0x80010008);
    assert(!hh_kane_override_begin(ram.data(),task,&original));
    write(ram.data(),data+0x24,0x10008);
    uint32_t field=hh_kane_override_begin(ram.data(),task,&original);
    assert(field==data+0x28 && original==0x8017BC50);
    assert(read(ram.data(),field)>=0x81000100 && read(ram.data(),field)<0x81200000);
    hh_kane_override_end(ram.data(),field,original);
    assert(read(ram.data(),field)==0x8017BC50);
    assert(read(ram.data(),data+0x24)==0x10008);
    write(ram.data(),task+0x2C,0x7FFFFFFC);
    assert(!hh_kane_override_begin(ram.data(),task,&original));
    // The original draw call receives a bounded six-command envelope. Later
    // geometry remains in order; ordinary DMA masking resumes after the model.
    uint32_t dl=0x800792F8,root=fixture.frames.front();
    write(ram.data(),dl,0xDE000000);write(ram.data(),dl+4,root);
    write(ram.data(),dl+8,0xDE000000);write(ram.data(),dl+12,0x80123450);
    write(ram.data(),0x8008D5BC,dl+16);
    assert(hh_kane_wrap_draw(ram.data(),dl,root,0x8017BC50,0x80202000));
    assert(read(ram.data(),0x8008D5BC)==dl+56);
    assert(read(ram.data(),dl)==0xDA380003 && read(ram.data(),dl+4)==0x80202000);
    assert(read(ram.data(),dl+8)==0xE0525464 && read(ram.data(),dl+12)==0x10000064);
    assert(read(ram.data(),dl+16)==0x6400002C && read(ram.data(),dl+20)==1);
    assert(read(ram.data(),dl+24)==0xDE000000 && read(ram.data(),dl+28)==root);
    assert(read(ram.data(),dl+32)==0x6400002C && read(ram.data(),dl+36)==0);
    assert(read(ram.data(),dl+40)==0xE0525464 && read(ram.data(),dl+44)==0x20000000);
    assert(read(ram.data(),dl+48)==0xDE000000 && read(ram.data(),dl+52)==0x80123450);
    assert(read(ram.data(),dl+56)==0xA5A5A5A5);
    uint32_t full=0x800692B0+0x10000-256-8;
    write(ram.data(),full,0xDE000000);write(ram.data(),full+4,root);
    write(ram.data(),0x8008D5BC,full+8);
    assert(!hh_kane_wrap_draw(ram.data(),full,root,0x8017BC50,0x80202000));
    assert(read(ram.data(),full+4)==0x8017BC50);
    assert(read(ram.data(),0x8008D5BC)==full+8);
    // An invalid archive with a tiny body must reject relocations before reading them.
    auto invalid=std::filesystem::temp_directory_path()/"kane-invalid-test.bin";
    uint32_t metadata[]={1,1,1,1,0,0};
    { std::ofstream f(invalid,std::ios::binary);f.write("KNDL",4);f.write(reinterpret_cast<const char*>(metadata),sizeof(metadata));f.put(0); }
    setenv("HH_KANE_MODEL_PATH",invalid.c_str(),1);
    Model rejected;rejected.load(ram.data());assert(rejected.frames.empty());
    std::filesystem::remove(invalid);
    std::puts("Kane model: 40 native lists, endian conversion, owned allocation, eligibility, pointer restoration and malformed archive checks passed.");
}
