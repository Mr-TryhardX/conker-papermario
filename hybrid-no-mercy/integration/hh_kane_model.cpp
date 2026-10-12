// Experimental model substitution, enabled explicitly by HH_KANE_IN_GAME.
// Allocation belongs to librecomp's mod heap; no guessed scratch RAM is used.
#include "librecomp/addresses.hpp"
#include "hh_kane_motion.hpp"
#include "hh_kane_preview.hpp"
#include "hh_battle_state.hpp"
#include "hh_kane_combat.hpp"
#include <cerrno>
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
    uint32_t address=0, active_root=0;
    std::vector<uint32_t> frames;
    void load(uint8_t* ram,const char* variable="HH_KANE_MODEL_PATH") {
        if(attempted)return;
        attempted=true;
        const char* path=std::getenv(variable);
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
Model walk_model;
Model preview_model;
uint32_t animation_table=0;
hh_battle_state::State actor_animation;
bool actor_animation_valid=false;
struct CombatModel {
    Model model;
    uint32_t resource=0;
    bool configured=false;
    const char* key_variable;
    const char* path_variable;
    void configure(){
        if(configured)return;configured=true;
        const char* value=std::getenv(key_variable);
        if(!value || *value<'0' || *value>'9')return;
        errno=0;char* end=nullptr;unsigned long parsed=std::strtoul(value,&end,0);
        if(errno || !end || *end || parsed>UINT32_MAX)return;
        resource=uint32_t(parsed);
    }
};
CombatModel combat_models[]={
    {{},0,false,"HH_KANE_PUNCH_RESOURCE","HH_KANE_PUNCH_MODEL_PATH"},
    {{},0,false,"HH_KANE_KICK_RESOURCE","HH_KANE_KICK_MODEL_PATH"},
    {{},0,false,"HH_KANE_REACTION_RESOURCE","HH_KANE_REACTION_MODEL_PATH"}
};
struct CombatPose {uint32_t pose,resource;size_t frame;};
std::optional<CombatPose> combat_pose(uint8_t* ram,uint32_t task){
    hh_battle_state::State state;
    if(!hh_battle_state::read(ram,task,state) || !state.player)return {};
    if(actor_animation_valid && actor_animation.task==task && actor_animation.data==state.data)state=actor_animation;
    for(auto& entry:combat_models){
        entry.configure();
        if(!entry.resource || state.resource!=entry.resource)continue;
        hh_battle_state::Metadata metadata;
        if(!hh_battle_state::metadata(ram,animation_table,state,metadata))return {};
        entry.model.load(ram,entry.path_variable);
        auto frame=kane_combat_frame(3,state.resource,entry.resource,state.frame(),metadata.frame_count,entry.model.frames.size());
        if(frame)return CombatPose{entry.model.frames[*frame],state.resource,*frame};
    }
    return {};
}
KaneMotion motion;
KanePreview preview;
KanePreviewButton preview_button;
uint32_t preview_request=0;
int64_t last_preview_poll=0;
void poll_preview_request(int64_t ms) {
    static const char* path=std::getenv("HH_KANE_PREVIEW_REQUEST_PATH");
    if(!path || preview_model.frames.empty() || ms-last_preview_poll<50)return;
    last_preview_poll=ms;
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()<=0 || file.tellg()>64)return;
    file.seekg(0);
    file>>std::ws;
    if(file.peek()<'0' || file.peek()>'9')return;
    uint32_t request;char extra;
    if((file>>request) && !(file>>extra))preview_request=request;
}
}
extern "C" void hh_kane_set_animation_table(uint32_t table){animation_table=table;}
extern "C" void hh_kane_animation_update(uint8_t* ram,uint32_t actor,uint32_t animation){
    hh_battle_state::State state;
    if(!hh_battle_state::actor_animation(ram,actor,animation,state))return;
    actor_animation=state;actor_animation_valid=true;
    static const char* path=std::getenv("HH_ANIMATION_UPDATE_PROBE_PATH");
    static hh_battle_state::Changes changes;
    if(path && *path && changes.accept(state)){
        auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if(FILE* f=std::fopen(path,"a")){
            std::fprintf(f,"{\"host_ms\":%lld,\"task\":%u,\"actor\":%u,\"animation\":%u,\"bank\":%u,\"clip\":%u,\"frame\":%u,\"flags_raw\":%u,\"speed_bits_raw\":%u}\n",static_cast<long long>(ms),state.task,actor,animation,state.bank(),state.clip(),state.frame(),state.frame_flags,state.speed_bits);
            std::fclose(f);
        }
    }
}
extern "C" uint32_t hh_kane_override_begin(uint8_t* ram,uint32_t task,uint32_t* original) {
    hh_battle_state::observe(ram,task);
    hh_battle_state::snapshot(ram,task);
    static const bool enabled=std::getenv("HH_KANE_IN_GAME")!=nullptr;
    if(!enabled || !in_guest(task,0x30) || read(ram,0x801BBCD0u)!=task)return 0;
    uint32_t data=read(ram,task+0x2C);
    if(!in_guest(data,0x70))return 0;
    uint32_t flags=read(ram,data+0x24),geometry=read(ram,data+0x28);
    if((flags&0x80000000u) || !geometry)return 0;
    model.load(ram);
    if(model.frames.empty())return 0;
    auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    float x,z;
    uint32_t bits=read(ram,data+4);std::memcpy(&x,&bits,4);
    bits=read(ram,data+12);std::memcpy(&z,&bits,4);
    bool moving=motion.sample(task,data,x,z,ms);
    walk_model.load(ram,"HH_KANE_WALK_MODEL_PATH");
    uint32_t pose=model.frames[(ms/120)%model.frames.size()];
    bool walking=moving && !walk_model.frames.empty();
    size_t frame=(ms/120)%model.frames.size();
    if(walking) { frame=motion.frame(walk_model.frames.size());pose=walk_model.frames[frame]; }
    preview_model.load(ram,"HH_KANE_PREVIEW_MODEL_PATH");
    static const bool button_preview=[] {
        const char* value=std::getenv("HH_KANE_PREVIEW_BUTTON");
        return value && std::strcmp(value,"1")==0;
    }();
    bool preview_allowed=true;
    if(button_preview) {
        uint16_t held=0;
        std::memcpy(&held,ram+((0x1BBBF0u+0x9Eu)^2u),sizeof(held));
        uint8_t camera_mode=ram[(0x1BBBF0u+0x29Au)^3u];
        preview_allowed=camera_mode==2;
        auto request=preview_button.sample(task,(held&0x20u)!=0,preview_allowed);
        if(request)preview_request=*request;
    } else poll_preview_request(ms);
    auto preview_frame=preview.sample(task,preview_request,ms,preview_model.frames.size(),preview_allowed);
    unsigned mode=walking?1:0;
    if(preview_frame) { mode=2;frame=*preview_frame;pose=preview_model.frames[frame]; }
    auto combat=combat_pose(ram,task);
    if(combat){mode=3;frame=combat->frame;pose=combat->pose;}
    static const char* probe=std::getenv("HH_KANE_MOTION_PROBE_PATH");
    static bool observed=false;
    static unsigned last_mode=0;
    static uint32_t last_task=0;
    static unsigned records=0;
    if(probe && std::isfinite(x) && std::isfinite(z) && records<128 && (!observed || last_task!=task || mode!=last_mode)) {
        if(FILE* file=std::fopen(probe,"a")) {
            std::fprintf(file,"{\"task\":%u,\"mode\":\"%s\",\"frame\":%zu,\"resource\":%u,\"x\":%.9g,\"z\":%.9g}\n",task,mode==3?"combat":mode==2?"preview":mode==1?"walk":"idle",frame,combat?combat->resource:0,x,z);
            std::fclose(file);++records;
        }
        observed=true;last_task=task;last_mode=mode;
    }
    *original=geometry;
    write(ram,data+0x28,pose);
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
        model.active_root=read(ram,0x801BBCD0u);
        return true;
    }
    // Never leave an unresolved heap pointer in the ordinary segmented path.
    for(uint32_t pos=before;pos<after;pos+=8)
        if(read(ram,pos)==0xDE000000u && read(ram,pos+4)==root)write(ram,pos+4,original);
    return false;
}

extern "C" int hh_kane_hide_descendant(uint8_t* ram,uint32_t task) {
    if(model.frames.empty() || !model.active_root || task==model.active_root ||
       read(ram,0x801BBCD0u)!=model.active_root)return 0;
    uint32_t root_data=read(ram,model.active_root+0x2C);
    if(!in_guest(root_data,0x70) || !read(ram,root_data+0x28))return 0;
    // Task creation in func_80005670 establishes +0xC as the parent link.
    // Bound ancestry traversal so corrupt pointers/cycles cannot stall rendering.
    for(unsigned depth=0;depth<32;++depth) {
        if(!in_guest(task,0x30))return 0;
        task=read(ram,task+0xC);
        if(task==model.active_root)return 1;
        if(!task)return 0;
    }
    return 0;
}
extern "C" void hh_kane_discard_draw(uint8_t* ram,uint32_t before) {
    uint32_t after=read(ram,0x8008D5BCu);
    if(in_guest(before,8) && before%8==0 && after>=before && after-before<=4096) {
        write(ram,0x8008D5BCu,before);
        write(ram,0x8008D548u,0);
    }
}
