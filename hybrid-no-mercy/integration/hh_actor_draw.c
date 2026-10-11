// Opt-in observation at the verified kind-6 draw emitter. Original code is
// regenerated locally from the user's game; it is not distributed here.
#include "recomp.h"
#include "trace.h"
#include "../../RecompiledFuncs/funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hh_actor_draw_original.inc"

extern uint32_t hh_kane_override_begin(uint8_t*, uint32_t, uint32_t*);
extern void hh_kane_override_end(uint8_t*, uint32_t, uint32_t);
extern int hh_kane_wrap_draw(uint8_t*, uint32_t, uint32_t, uint32_t, uint32_t);

extern int hh_kane_hide_descendant(uint8_t*, uint32_t);
extern void hh_kane_discard_draw(uint8_t*, uint32_t);

static int actor_word(const uint8_t* ram, uint32_t address, uint32_t* out) {
    if (address < 0x80000000u || address > 0x807FFFFCu || (address & 3)) return 0;
    memcpy(out, ram + (address - 0x80000000u), 4);
    return 1;
}
// Deliberately strong: generated base definitions are weak. This wrapper always
// executes the unmodified original function with the same guest context.
void func_8000C768_D368(uint8_t* rdram, recomp_context* ctx) {
    static int initialized = 0;
    static const char* path;
    static unsigned count;
    // Research output must remain bounded across overnight sessions. The
    // original emitter and model override still run after this limit.
    static const unsigned record_limit = 32768;
    static uint32_t seen_tasks[128];
    static unsigned task_counts[128];
    static uint32_t observed_player;
    if (!initialized) { path = getenv("HH_ACTOR_DRAW_PROBE_PATH"); initialized = 1; }
    uint32_t task = (uint32_t)ctx->r4, player = 0, before = 0;
    int observe = count < record_limit && path && *path && task >= 0x80000000u && task <= 0x807FFFC0u;
    if (observe) actor_word(rdram, 0x801BBCD0u, &player);
    // Cutscene actor hierarchies can fill this bounded table before the field
    // player exists. Reclaim observation slots when the game's player task
    // changes, instead of silently excluding newly entered gameplay actors.
    if (observe && player != observed_player) {
        memset(seen_tasks, 0, sizeof(seen_tasks));
        memset(task_counts, 0, sizeof(task_counts));
        observed_player = player;
    }
    actor_word(rdram, 0x8008D5BCu, &before);
    uint32_t original = 0;
    uint32_t replacement_field = hh_kane_override_begin(rdram, task, &original);
    uint32_t model_root = 0;
    if (replacement_field) actor_word(rdram, replacement_field, &model_root);
    int hide_original_skin = hh_kane_hide_descendant(rdram, task);
    hh_original_actor_draw(rdram, ctx);
    if (hide_original_skin) hh_kane_discard_draw(rdram, before);
    if (replacement_field) {
        uint32_t matrix = 0;
        actor_word(rdram, task + 0x1C, &matrix);
        hh_kane_wrap_draw(rdram, before, model_root, original, matrix);
    }
    hh_kane_override_end(rdram, replacement_field, original);
    unsigned slot = 0;
    if (observe) {
        while (slot < 128 && seen_tasks[slot] && seen_tasks[slot] != task) ++slot;
        // Do not churn the last slot and write on every draw when the table fills.
        if (slot == 128) {
            if (player == task) slot = 127; // retain the active player after scene changes
            else observe = 0;
        }
        if (observe && seen_tasks[slot] != task) { seen_tasks[slot] = task; task_counts[slot] = 0; }
    }
    if (observe && (++task_counts[slot] == 1 || task_counts[slot] % 60 == 0)) {
        ++count;
        uint32_t after = 0, data = 0;
        actor_word(rdram, 0x8008D5BCu, &after);
        actor_word(rdram, task + 0x2C, &data);
        FILE* file = fopen(path, "a");
        if (file) {
            fprintf(file, "{\"draw\":%u,\"task\":%u,\"data\":%u,\"dl_before\":%u,\"dl_after\":%u,\"data_words\":[", count, task, data, before, after);
            for (unsigned i = 0; i < 32; ++i) {
                uint32_t value = 0;
                int valid = data >= 0x80000000u && data <= 0x807FFF80u && actor_word(rdram, data + i * 4, &value);
                if (valid) fprintf(file, "%s%u", i ? "," : "", value);
                else fprintf(file, "%snull", i ? "," : "");
            }
            fprintf(file, "],\"is_player_candidate\":%s,\"dl_words\":[", player == task ? "true" : "false");
            unsigned words = after >= before && after - before <= 4096 ? (after - before) / 4 : 0;
            for (unsigned i = 0; i < words; ++i) {
                uint32_t value;
                if (!actor_word(rdram, before + i * 4, &value)) break;
                fprintf(file, "%s%u", i ? "," : "", value);
            }
            fputs("]}\n", file);
            fclose(file);
        }
    }
}
