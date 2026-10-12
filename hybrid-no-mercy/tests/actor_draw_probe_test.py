#!/usr/bin/env python3
"""Exercise the authored observer against a stub original emitter, not game code.

The native target is built separately. This fixture verifies transparent calls,
log growth limits, raw halfword endian handling, and matrix pointer bounds.
"""
from pathlib import Path
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'integration/hh_actor_draw.c').read_text()
source = '\n'.join(line for line in source.splitlines()
                   if not line.startswith('#include "'))
prefix = r'''
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
typedef struct { uint64_t r4; } recomp_context;
static unsigned original_calls;
static int32_t* section_addresses;
static void hh_original_actor_draw(uint8_t* ram, recomp_context* ctx) {
    (void)ram; (void)ctx; ++original_calls;
}
'''
suffix = r'''
uint32_t hh_kane_override_begin(uint8_t*a,uint32_t b,uint32_t*c){return 0;}
void hh_kane_set_animation_table(uint32_t table){}
void hh_kane_override_end(uint8_t*a,uint32_t b,uint32_t c){}
int hh_kane_wrap_draw(uint8_t*a,uint32_t b,uint32_t c,uint32_t d,uint32_t e){return 0;}
int hh_kane_hide_descendant(uint8_t*a,uint32_t b){return 0;}
void hh_kane_discard_draw(uint8_t*a,uint32_t b){}
int main(int argc, char** argv) {
    if(argc!=4)return 2;
    uint8_t* ram=calloc(0x800000,1);
    if(!ram)return 3;
    recomp_context ctx={.r4=0x80010000};
    uint32_t task=ctx.r4, matrix=(uint32_t)strtoul(argv[2],0,0);
    uint16_t counter=65535;
    memcpy(ram+0x1BBCD0,&task,4);
    memcpy(ram+0x1001C,&matrix,4);
    memcpy(ram+(0x89358^2),&counter,2);
    for(unsigned i=0;i<16;++i) {
        uint32_t word=i+1;
        memcpy(ram+0x20000+i*4,&word,4);
    }
    if(setenv("HH_ACTOR_DRAW_PROBE_PATH",argv[1],1))return 4;
    uint32_t child=0x80010100;
    int tree=strcmp(argv[3],"tree")==0;
    if(tree) {
        memcpy(ram+0x10008,&child,4); memcpy(ram+0x10010,&child,4);
        memcpy(ram+0x1010C,&task,4); memcpy(ram+0x1011C,&matrix,4);
    }
    uint8_t* before=malloc(0x800000);
    if(!before)return 6;
    memcpy(before,ram,0x800000);
    unsigned calls=tree ? 234 : (unsigned)strtoul(argv[3],0,0);
    if(tree) {
        for(unsigned i=1;i<=120;++i) {
            ctx.r4=task;func_8000C768_D368(ram,&ctx);
            // The child starts later, so its own per-task stride differs.
            if(i>=7) {ctx.r4=child;func_8000C768_D368(ram,&ctx);}
        }
    } else for(unsigned i=0;i<calls;++i)func_8000C768_D368(ram,&ctx);
    int unchanged=memcmp(before,ram,0x800000)==0;
    free(before);
    free(ram);
    return original_calls==calls && unchanged ? 0 : 5;
}
'''
with tempfile.TemporaryDirectory(prefix='hh-observer-') as folder:
    folder = Path(folder)
    fixture = folder / 'fixture.c'
    fixture.write_text(prefix + source + suffix)
    binary = folder / 'fixture'
    subprocess.run(['cc', '-std=c11', '-O2', str(fixture), '-o', str(binary)], check=True)
    for index, pointer in enumerate([0x80020000, 0, 0x7FFFFFFF, 0x80020001, 0x807FFFFC, 0x80800000]):
        output = folder / f'{index}.jsonl'
        calls = 2_000_000 if index == 0 else 2
        subprocess.run([str(binary), str(output), hex(pointer), str(calls)], check=True)
        with output.open() as stream:
            record = json.loads(next(stream))
            lines = 1 + sum(1 for _ in stream)
        assert lines == (32768 if index == 0 else 1), (pointer, lines)
        assert record['update_counter'] == 65535
        assert record['player_draw_epoch'] == 1
        assert len(record['task_words']) == 12 and record['task_words'][7] == pointer
        assert record['matrix'] == pointer
        assert record['matrix_words'] == (list(range(1, 17)) if index == 0 else [None] * 16)
        assert record['data_words'] == [None] * 32
    output = folder / 'tree.jsonl'
    subprocess.run([str(binary), str(output), '0x80020000', 'tree'], check=True)
    records = [json.loads(line) for line in output.read_text().splitlines()]
    roots = [r['player_draw_epoch'] for r in records if r['is_player_candidate']]
    children = [r['player_draw_epoch'] for r in records if not r['is_player_candidate']]
    assert roots == [1, 60, 120] and children == [60, 120], (roots, children)
print('PASS: 2,000,000 original emitter calls continue through the 32,768-record cap.')
print('PASS: task topology, raw counter endian handling, and valid/invalid matrix pointer bounds.')
print('PASS: late-created children align with root sampling; guest bytes remain unchanged.')
