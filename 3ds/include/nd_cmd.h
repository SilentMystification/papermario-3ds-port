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
    NDC_MODEL = 3, /* D1: payload = the model record (G_PM_REC layout, gbi_custom.h) and its R13 words; rec_run reads it */
    NDC_CALL = 4,  /* D1: replay the commands from arg2 up to payload unit 0 word 0 (the floor reflection range) */
    NDC_SPR = 5,   /* D2: payload = the sprite record (G_PM_SPR, 4 pairs), then its gSPVertex and gSP2Triangles words */
    NDC_ENT = 6,   /* D3: payload = an entity record (G_PM_ENT, 4 pairs; DRAW or END form), then for a DRAW record its G_DL word (5 pairs) */
    NDC_TEXRECT = 8, /* D4b-1: gSPTextureRectangle; payload unit 0: w0, w1; unit 1: the RDPHALF_1 and RDPHALF_2 words (s t, dsdx dtdy) */
    NDC_STATE = 9,   /* D4b-3: a run of state ops (othermode, combine, colours, geometry mode, texture, scissor, fill rectangle, tile and image ops); payload: one unit
                      * per op (its w0, w1). The replay runs each op with the walker's per-op rules (g2_same and g2_note, gv_epoch, the op handler). */
    NDC_N
};
#define NDC_HDR(op, units, arg) (((uint32_t)(op) << 24) | ((uint32_t)(units) << 16) | ((uint32_t)(arg) & 0xffffu))

/* Record side (nd_rec.c). */
const uint32_t* nd_rec_close(void); /* nuGfxTaskStart: NDC_END; returns the head of the task stream */

void nd_rec_model(const void* rec, unsigned words); /* D1: a model record of `words` Gfx pairs (record and R13 words) */
void nd_rec_spr(const void* rec, const void* geo); /* D2: a sprite record (4 pairs) and its 2 geometry words */
void nd_rec_ent_draw(const void* list, unsigned segid, unsigned varray, unsigned form); /* D3: the entity draw of a static list (record and its G_DL) */
void nd_rec_ent(const void* rec);                   /* D3: an entity record with no G_DL (the entcheck END record) */
/* D4b-1, D5: the texture rectangle (the nd_gbi.h redirect records it; only with nd_rec_sub == 0). */
void nd_rec_texrect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd);
void nd_rec_state(uint32_t w0, uint32_t w1); /* D4b-3: one state op, appended to the open NDC_STATE run (a new run after any other command) */
const uint32_t* nd_rec_pos(void);                   /* D4b-2: the stream write position (no cut): moves when any command is recorded */
const uint32_t* nd_rec_mark(void);                  /* D1: cut, then the stream position (a range start or end) */
void nd_rec_call(const uint32_t* a, const uint32_t* b); /* D1: replay the range [a, b) here */
/* D6c: inline data (the vertex buffers that the list held behind a branch). Returns 8-byte aligned memory that lives as long as the stream of
 * the context (the replay skips it: an NDC_JUMP over the data). In a list for later (nd_rec_sub > 0) the data goes into that list behind a
 * branch word, as before. */
void* nd_rec_data(unsigned bytes);
/* D6c: a list for later of an effect (built once, drawn by one or more calls) as a stream range. nd_sub_open records an NDC_JUMP over the range
 * (patched by nd_sub_close), nd_sub_call replays it (NDC_CALL). In a list for later (nd_rec_sub > 0): the list form (a branch word over the
 * sublist, its G_ENDDL, a G_DL push call). */
typedef struct { const uint32_t* a; const uint32_t* b; uint32_t* br; int list; } NdSub;
void nd_sub_open(NdSub* s);
void nd_sub_close(NdSub* s);
void nd_sub_call(const NdSub* s);
/* D1: a builder that writes a list for later (build_custom_gfx) runs with nd_rec_sub > 0: records go to that list, not to the stream. */
extern int nd_rec_sub;
/* D7: build_custom_gfx writes its lists for later into stream memory (nd_rec.c): begin returns the room (16 KB), end keeps the used part. */
void* nd_rec_list_begin(void);
void nd_rec_list_end(void* end);
int nd_rec_dyn(const void* p); /* D9: p is in frame stream memory (dynamic by address) */

#define ND_REC_AUX_MAX 96 /* the most R13 words of one model record (make_texture_gfx with mipmaps and two palettes) */
void nd_rec_cnt_take(unsigned o[NDC_N], unsigned* sw, unsigned* kb); /* PROF: commands per op and state ops (sw) since the last take; the stream high-water mark (KB) */
/* Walker side (nd_rec.c). */
void nd_stream_run(const uint32_t* head); /* replay one task stream */
#endif
