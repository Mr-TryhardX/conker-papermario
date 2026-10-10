#include "recomp.h"
#include "funcs.h"
#include <stdio.h>
#include <string.h>
int main(void){
    uint8_t* rdram=calloc(1,(int32_t)0x800000);uint8_t frame[98];
    uint64_t angle=0x3fd6800000000000ULL, half=0x3fe0000000000000ULL;
    memcpy(rdram+0x55bf8,&angle,8);memcpy(rdram+0x55c00,&half,8);
    /* LDC1 fetches guest words separately, so store word-swapped doubles. */
    MEM_W(0,(int32_t)0x80055bf8)=0x3fd68000;MEM_W(4,(int32_t)0x80055bf8)=0;
    MEM_W(0,(int32_t)0x80055c00)=0x3fe00000;MEM_W(4,(int32_t)0x80055c00)=0;
    while(fread(frame,1,98,stdin)==98){
        for(int i=0;i<98;i++)MEM_B(i,(int32_t)0x80001000)=frame[i];
        memset(rdram+0x69f10,0,12);
        uint32_t pointer=(int32_t)0x80001000;
        for(int joint=0;joint<19;joint++){
            recomp_context ctx={0};ctx.r4=(int32_t)pointer;ctx.r5=(int32_t)0x80002000;ctx.r6=(int32_t)0x80002010;
            for(int i=0;i<3;i++)MEM_W(i*4,(int32_t)0x80002010)=0x7fc00000;
            func_800135D0(rdram,&ctx);pointer=(uint32_t)ctx.r2;
            for(int i=0;i<3;i++){uint32_t word=MEM_W(i*4,(int32_t)0x80002000);fwrite(&word,4,1,stdout);}
            for(int i=0;i<3;i++){uint32_t word=MEM_W(i*4,(int32_t)0x80002010);fwrite(&word,4,1,stdout);}
        }
        if(pointer!=0x80001062)return 3;
    }
    free(rdram);return 0;
}
