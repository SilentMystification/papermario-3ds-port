/* nd_cmd.h: the frame stream (DIRECT, docs/handoff/plan_direct.md section 2). The game (logic thread, or the game thread in serial mode)
 * records native commands for each graphics task; the walker side (core 0) replays them in order. A command is one 8-byte header plus
 * 0 or more 8-byte payload units, 8-byte aligned:
 *   word 0: op << 24 | units << 16 | arg (16 bits)    word 1: arg2 (a pointer or a value)
 * The stream of a display context lives in a chain of heap chunks (nd_rec.c), reset when that context is built again. The replay reads the
 * stream and writes nothing into it (replay rule), so eye 2 and the interp key frame replay the same stream. */
#ifndef ND_CMD_H
#define ND_CMD_H
#include <stdint.h>

enum {
    NDC_END = 0,  /* end of the task stream */
    NDC_JUMP = 1, /* arg2: the next chunk (the rest of this chunk is unused) */
    NDC_LIST = 2, /* bridge (D0 to D9): arg2 = first word of a span of the task list; payload unit 0 word 0 = byte count. arg 1: the first span of the task (a full sync, as a new list) */
    NDC_MODEL = 3, /* D1: payload = the model record (G_PM_REC layout, gbi_custom.h) and its R13 words; rec_run reads it */
    NDC_CALL = 4,  /* D1: replay the commands from arg2 up to payload unit 0 word 0 (the floor reflection range) */
    NDC_SPR = 5,   /* D2: payload = the sprite record (G_PM_SPR, 4 pairs), then its gSPVertex and gSP2Triangles words */
    NDC_N
};
#define NDC_HDR(op, units, arg) (((uint32_t)(op) << 24) | ((uint32_t)(units) << 16) | ((uint32_t)(arg) & 0xffffu))

/* Record side (nd_rec.c). */
void nd_rec_cut(void); /* close the span of words written since the last cut (every nd_rec_* call starts with it) */
const uint32_t* nd_rec_close(const void* list, unsigned nbytes); /* nuGfxTaskStart: the last span and NDC_END; returns the head of the task stream */
void nd_rec_model(const void* rec, unsigned words); /* D1: a model record of `words` Gfx pairs (record and R13 words) */
void nd_rec_spr(const void* rec, const void* geo); /* D2: a sprite record (4 pairs) and its 2 geometry words */
const uint32_t* nd_rec_mark(void);                  /* D1: cut, then the stream position (a range start or end) */
void nd_rec_call(const uint32_t* a, const uint32_t* b); /* D1: replay the range [a, b) here */
/* D1: a builder that writes a list for later (build_custom_gfx) runs with nd_rec_sub > 0: records go to that list, not to the stream. */
extern int nd_rec_sub;
#define ND_REC_SUB(call) do { nd_rec_sub++; call; nd_rec_sub--; } while (0)
#define ND_REC_AUX_MAX 96 /* the most R13 words of one model record (make_texture_gfx with mipmaps and two palettes) */
/* Walker side (nd_rec.c). */
void nd_stream_run(const uint32_t* head); /* replay one task stream */
#endif
