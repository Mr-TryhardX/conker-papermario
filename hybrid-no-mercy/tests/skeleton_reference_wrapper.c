#include "recomp.h"
#include "funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Research only: execute the original matrix builder in a private copy of
 * No Mercy memory. Never share this address space with Hybrid Heaven. */
int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) return 2;
    unsigned actor = 0;
    if (argc >= 3) {
        if (argv[2][0] < '0' || argv[2][0] > '3' || argv[2][1]) return 2;
        actor = argv[2][0] - '0';
    }
    int positions = argc == 4;
    if (positions && strcmp(argv[3], "--positions")) return 2;
    FILE *input = fopen(argv[1], "rb");
    if (!input) return 3;
    uint8_t *logical = malloc(0x800000), *rdram = malloc(0x800000);
    if (!logical || !rdram) return 4;
    if (fread(logical, 1, 0x800000, input) != 0x800000 || fgetc(input) != EOF) return 5;
    fclose(input);
    for (size_t i = 0; i < 0x800000; i++) rdram[i ^ 3] = logical[i];
    recomp_context ctx = {0};
    ctx.f_odd = &ctx.f0.u32h; /* FR=0: odd registers alias even upper words. */
    ctx.r29 = (int32_t)0x807FF000;
    ctx.r4 = actor;
    func_80012BE0(rdram, &ctx);
    /* The deformation-matrix translation is NOT the joint pivot. The original
     * builder resolves model-space pivots at pose slot +0x0C. */
    for (unsigned joint = positions ? 1 : 0; joint < 20; joint++) {
        for (unsigned cell = 0; cell < (positions ? 3u : 16u); cell++) {
            uint32_t address = positions
                ? 0x800A1130 + actor * 2852 + joint * 124 + 12 + cell * 4
                : 0x80091CC0 + actor * 0x1810 + joint * 0xDC + 0x1C + cell * 4;
            uint32_t bits = MEM_W(0, (int32_t)address);
            if (fwrite(&bits, 4, 1, stdout) != 1) return 6;
        }
    }
    free(logical); free(rdram);
    return 0;
}
