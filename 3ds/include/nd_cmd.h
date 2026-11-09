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
    NDC_ENT = 6,   /* D3: payload = an entity record (G_PM_ENT, 4 pairs; DRAW or END form), then for a DRAW record its G_DL word (5 pairs) */
    NDC_TEX4 = 7,  /* D4b-1: a compound texture load (gDPLoadTLUT_pal16, gDPLoadTextureBlock_4b, gDPLoadTextureTile_4b). arg bit 0: the palette load,
                    * bit 1: the 4 bit load, bit 2: its tile form (else block), bit 3: the load is gDPLoadTextureTile of size unit 2 word 1. arg2 = the raster; unit 0: palette, width | height << 16; unit 1: uls | ult << 16,
                    * lrs | lrt << 16 (texels); unit 2 word 0: fmt | cms << 4 | cmt << 6 | masks << 8 | maskt << 12 | shifts << 16 | shiftt << 20 | tile palette << 24 | palette slot << 28 */
    NDC_TEXRECT = 8, /* D4b-1: gSPTextureRectangle; payload unit 0: w0, w1; unit 1: the RDPHALF_1 and RDPHALF_2 words (s t, dsdx dtdy) */
    NDC_STATE = 9,   /* D4b-3: a run of state ops (othermode, combine, colours, geometry mode, texture, scissor, fill rectangle, tile and image ops); payload: one unit
                      * per op (its w0, w1). The replay runs each op with the walker's per-op rules (g2_same and g2_note, gv_epoch, the op handler). */
    NDC_N
};
#define NDC_HDR(op, units, arg) (((uint32_t)(op) << 24) | ((uint32_t)(units) << 16) | ((uint32_t)(arg) & 0xffffu))

/* Record side (nd_rec.c). */
void nd_rec_cut(void); /* close the span of words written since the last cut (every nd_rec_* call starts with it) */
const uint32_t* nd_rec_close(const void* list, unsigned nbytes); /* nuGfxTaskStart: the last span and NDC_END; returns the head of the task stream */
void nd_rec_model(const void* rec, unsigned words); /* D1: a model record of `words` Gfx pairs (record and R13 words) */
void nd_rec_spr(const void* rec, const void* geo); /* D2: a sprite record (4 pairs) and its 2 geometry words */
void nd_rec_ent_draw(const void* list, unsigned segid, unsigned varray, unsigned form); /* D3: the entity draw of a static list (record and its G_DL) */
void nd_rec_ent(const void* rec);                   /* D3: an entity record with no G_DL (the entcheck END record) */
/* D4b-1: the record forms of the compound texture loads and the texture rectangle (nd_gbi_tex.h redirects the macros of a file to them). Only with
 * nd_rec_sub == 0: the redirect writes the words into a list for later. */
typedef struct NdTex4 { const void* raster; const void* pal; uint32_t flags, wh, ul, lr, f, siz; } NdTex4; /* the NDC_TEX4 fields (arg, arg2, units) */
void nd_rec_tex4(const NdTex4* t);
void nd_rec_texrect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd);
void nd_rec_state(uint32_t w0, uint32_t w1); /* D4b-3: one state op, appended to the open NDC_STATE run (a new run after any other command) */
const uint32_t* nd_rec_pos(void);                   /* D4b-2: the stream write position (no cut): moves when any command is recorded */
const uint32_t* nd_rec_mark(void);                  /* D1: cut, then the stream position (a range start or end) */
void nd_rec_call(const uint32_t* a, const uint32_t* b); /* D1: replay the range [a, b) here */
/* D1: a builder that writes a list for later (build_custom_gfx) runs with nd_rec_sub > 0: records go to that list, not to the stream. */
extern int nd_rec_sub;
#define ND_REC_SUB(call) do { nd_rec_sub++; call; nd_rec_sub--; } while (0)
#define ND_REC_AUX_MAX 96 /* the most R13 words of one model record (make_texture_gfx with mipmaps and two palettes) */
void nd_rec_cnt_take(unsigned o[NDC_N], unsigned* sw, unsigned* kb); /* PROF: commands per op and state ops (sw) since the last take; the stream high-water mark (KB) */
/* Walker side (nd_rec.c). */
void nd_stream_run(const uint32_t* head); /* replay one task stream */
#endif
