#!/usr/bin/env python3
"""Verify the observation wrapper preserves original inputs and call ordering."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source='\n'.join(line for line in (root/'integration/hh_actor_animation.c').read_text().splitlines() if not line.startswith('#include "'))
fixture=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef struct {uint64_t r4,r5;} recomp_context;
static unsigned original_calls, observed_calls;
static void hh_original_actor_animation(uint8_t* ram,recomp_context* ctx){
    ++original_calls;ram[0]=37;ctx->r4=123;ctx->r5=456;
}
'''+source+r'''
void hh_kane_animation_update(uint8_t* ram,uint32_t actor,uint32_t animation){
    assert(original_calls==observed_calls+1);assert(ram[0]==37);
    assert(actor==0x80200000 && animation==0x80201000);++observed_calls;
}
int main(){
    uint8_t ram[2]={0,91};
    for(unsigned i=0;i<1000;++i){
        recomp_context ctx={.r4=0x80200000,.r5=0x80201000};
        func_8000F458_10058(ram,&ctx);
        assert(ctx.r4==123 && ctx.r5==456 && ram[1]==91);
    }
    assert(original_calls==1000 && observed_calls==1000);
    puts("PASS: original animation builder runs once before observation; original context and guest effects are preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='hh-animation-wrapper-') as path:
    path=Path(path);(path/'fixture.c').write_text(fixture)
    subprocess.run(['cc','-std=c11','-O2',str(path/'fixture.c'),'-o',str(path/'fixture')],check=True)
    subprocess.run([str(path/'fixture')],check=True)
