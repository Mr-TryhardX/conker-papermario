// Observe the original actor-animation builder without altering game state.
#include "recomp.h"
#include "trace.h"
#include "../../RecompiledFuncs/funcs.h"
#include "hh_actor_animation_original.inc"
extern void hh_kane_animation_update(uint8_t*,uint32_t,uint32_t);
void func_8000F458_10058(uint8_t* rdram,recomp_context* ctx){
    uint32_t actor=(uint32_t)ctx->r4,animation=(uint32_t)ctx->r5;
    hh_original_actor_animation(rdram,ctx);
    hh_kane_animation_update(rdram,actor,animation);
}
