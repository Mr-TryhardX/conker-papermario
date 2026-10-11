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
static int actor_in_player_tree(const uint8_t* ram, uint32_t task, uint32_t player) {
    if (!player) return 0;
    for (unsigned depth = 0; depth < 32; ++depth) {
        if (task == player) return 1;
        if (task < 0x80000000u || task > 0x807FFFC0u || (task & 3)) return 0;
        if (!actor_word(ram, task + 0xC, &task) || !task) return 0;
    }
    return 0;
}
// Deliberately strong: the installer renames the generated base definition
// because COFF weak inline linkage cannot safely interpose it. All original
// references resolve here; the exact original body runs with the same context.
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
    static uint64_t player_draw_epoch;
    static uint64_t player_draws_for_binding;
    static int capture_player_epoch;
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
        player_draws_for_binding = 0;
        capture_player_epoch = 0;
    }
    if (observe && task == player) {
        ++player_draw_epoch;
        ++player_draws_for_binding;
        capture_player_epoch = player_draws_for_binding == 1 || player_draws_for_binding % 60 == 0;
    }
    int player_tree = observe && actor_in_player_tree(rdram, task, player);
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
    unsigned task_count = observe ? ++task_counts[slot] : 0;
    // Sample a whole player draw together; independent per-part strides can
    // otherwise combine poses from different updates. Other actors stay sparse.
    if (observe && (player_tree ? capture_player_epoch : (task_count == 1 || task_count % 60 == 0))) {
        ++count;
        uint32_t after = 0, data = 0;
        actor_word(rdram, 0x8008D5BCu, &after);
        actor_word(rdram, task + 0x2C, &data);
        FILE* file = fopen(path, "a");
        if (file) {
            uint16_t update_counter = 0;
            uint32_t matrix = 0;
            // Original title/main-loop counter (lhu at 80001A64, increment
            // at 80001A74). It can stall during gameplay; never use it as the
            // pose-group key. Keep the raw wraparound value for diagnostics.
            memcpy(&update_counter, rdram + (0x89358u ^ 2u), 2);
            actor_word(rdram, task + 0x1C, &matrix);
            fprintf(file, "{\"draw\":%u,\"player_draw_epoch\":%llu,\"update_counter\":%u,\"task\":%u,\"data\":%u,\"matrix\":%u,\"dl_before\":%u,\"dl_after\":%u,\"task_words\":[", count, (unsigned long long)player_draw_epoch, update_counter, task, data, matrix, before, after);
            for (unsigned i = 0; i < 12; ++i) {
                uint32_t value = 0;
                actor_word(rdram, task + i * 4, &value);
                fprintf(file, "%s%u", i ? "," : "", value);
            }
            fputs("],\"matrix_words\":[", file);
            for (unsigned i = 0; i < 16; ++i) {
                uint32_t value = 0;
                int valid = matrix >= 0x80000000u && matrix <= 0x807FFFC0u &&
                            actor_word(rdram, matrix + i * 4, &value);
                if (valid) fprintf(file, "%s%u", i ? "," : "", value);
                else fprintf(file, "%snull", i ? "," : "");
            }
            fputs("],\"data_words\":[", file);
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
