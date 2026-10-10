// Experimental model substitution, enabled explicitly by HH_KANE_IN_GAME.
// Allocation belongs to librecomp's mod heap; no guessed scratch RAM is used.
#include "librecomp/addresses.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdint>

namespace {
uint32_t word(const uint8_t* p) { uint32_t x; std::memcpy(&x,p,4); return x; }
uint32_t be(const uint8_t* p) { return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3]; }
bool in_guest(uint32_t a, uint32_t n) { return a>=0x80000000u && n<=0x800000u && a-0x80000000u<=0x800000u-n && !(a&3); }
uint32_t read(const uint8_t* ram,uint32_t a) { return word(ram+a-0x80000000u); }
void write(uint8_t* ram,uint32_t a,uint32_t x) { std::memcpy(ram+a-0x80000000u,&x,4); }
struct Model {
    bool attempted=false;
    uint32_t address=0;
    std::vector<uint32_t> frames;
    void load(uint8_t* ram) {
        if(attempted)return;
        attempted=true;
        const char* path=std::getenv("HH_KANE_MODEL_PATH");
        if(!path)return;
        std::ifstream file(path,std::ios::binary | std::ios::ate);
        if(!file)return;
        auto length=file.tellg();
        // Bound the archive before allocating host memory, including metadata.
        if(length<20 || length>8*1024*1024+20+4*(120+100000))return;
        file.seekg(0);
        std::vector<uint8_t> bytes(static_cast<size_t>(length));
        if(!file.read(reinterpret_cast<char*>(bytes.data()),length))return;
        if(bytes.size()<20 || std::memcmp(bytes.data(),"KNDL",4) || word(bytes.data()+4)!=1)return;
        uint32_t count=word(bytes.data()+8),size=word(bytes.data()+12),relocs=word(bytes.data()+16);
        if(!count || count>120 || size>8*1024*1024 || size<8 || size%8 || relocs>100000)return;
        size_t header=20+size_t(count+relocs)*4;
        if(header>bytes.size() || bytes.size()-header!=size)return;
        const uint8_t* body=bytes.data()+header;
        for(uint32_t i=0;i<count;++i) { uint32_t offset=word(bytes.data()+20+i*4); if(offset%8 || offset>=size)return; }
        for(uint32_t i=0;i<relocs;++i) {
            uint32_t offset=word(bytes.data()+20+(count+i)*4);
            if(offset%4 || offset>size-4 || be(body+offset)>=size)return;
        }
        auto* allocation=static_cast<uint8_t*>(recomp::alloc(ram,size));
        if(!allocation)return;
        size_t physical=allocation-ram;
        if(physical<0x1000000 || physical>0x4000000-size) { recomp::free(ram,allocation); return; }
        address=0x80000000u+static_cast<uint32_t>(physical);
        for(uint32_t i=0;i<size;++i) ram[(physical+i)^3]=body[i];
        for(uint32_t i=0;i<relocs;++i) {
            uint32_t offset=word(bytes.data()+20+(count+i)*4);
            uint32_t value=be(body+offset)+address;
            std::memcpy(allocation+offset,&value,4);
        }
        for(uint32_t i=0;i<count;++i)frames.push_back(address+word(bytes.data()+20+i*4));
        std::fprintf(stderr,"[Kane] loaded %u native reference poses, %u bytes at %08X\n",count,size,address);
    }
};
Model model;
}
extern "C" uint32_t hh_kane_override_begin(uint8_t* ram,uint32_t task,uint32_t* original) {
    static const bool enabled=std::getenv("HH_KANE_IN_GAME")!=nullptr;
    if(!enabled || !in_guest(task,0x30) || read(ram,0x801BBCD0u)!=task)return 0;
    uint32_t data=read(ram,task+0x2C);
    if(!in_guest(data,0x70))return 0;
    uint32_t flags=read(ram,data+0x24),geometry=read(ram,data+0x28);
    if((flags&0x80000000u) || !geometry)return 0;
    model.load(ram);
    if(model.frames.empty())return 0;
    auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    *original=geometry;
    write(ram,data+0x28,model.frames[(ms/120)%model.frames.size()]);
    // The custom list uses its own material; invalidate the renderer's material cache.
    write(ram,0x800892B0u+0x4298u,0);
    return data+0x28;
}
extern "C" void hh_kane_override_end(uint8_t* ram,uint32_t field,uint32_t original) {
    if(!field)return;
    if(in_guest(field,4))write(ram,field,original);
    write(ram,0x800892B0u+0x4298u,0);
}

// Wrap only the owned model call in supported RT64 extended-address commands.
// Original materials, transforms, framebuffer addresses and other actors keep
// ordinary N64 DMA masking. The frame's existing display-list buffers are
// established by main_27C0: alternating roots separated by 0x10048 bytes.
extern "C" int hh_kane_wrap_draw(uint8_t* ram,uint32_t before,uint32_t root,uint32_t original,uint32_t matrix) {
    uint32_t after=read(ram,0x8008D5BCu);
    uint32_t start=before>=0x800792F8u ? 0x800792F8u : 0x800692B0u;
    uint32_t limit=start+0x10000u;
    if(!in_guest(before,8) || !in_guest(after,4) || before%8 || after<before || after%8 || after-before>4096)return 0;
    bool room=before>=start && after<=limit-256-40;
    for(uint32_t pos=before;pos<after;pos+=8) {
        if(read(ram,pos)!=0xDE000000u || read(ram,pos+4)!=root)continue;
        if(!room || !in_guest(matrix,64)) { write(ram,pos+4,original);return 0; }
        std::memmove(ram+pos+48-0x80000000u,ram+pos+8-0x80000000u,after-pos-8);
        const uint32_t commands[]={0xDA380003u,matrix,0xE0525464u,0x10000064u,0x6400002Cu,1u,0xDE000000u,root,0x6400002Cu,0u,0xE0525464u,0x20000000u};
        for(unsigned i=0;i<12;++i)write(ram,pos+i*4,commands[i]);
        write(ram,0x8008D5BCu,after+40);
        return true;
    }
    // Never leave an unresolved heap pointer in the ordinary segmented path.
    for(uint32_t pos=before;pos<after;pos+=8)
        if(read(ram,pos)==0xDE000000u && read(ram,pos+4)==root)write(ram,pos+4,original);
    return false;
}
