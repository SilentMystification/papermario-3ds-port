/* nd_api.h: the native draw API skeleton (NATIVEAPI, plan_nativeapi.md S0).
 * One NDC_NAPI command per draw: word 1 = the core 0 runner (a function pointer), payload = its input record. stream_run (nd_rec.c)
 * calls the runner, so a new producer never edits nd_rec.c. Pso table: nd_pso_n64 gives an id per distinct N64 state description;
 * an entry never changes after its id is returned. Recorded on core 1, executed on core 0.
 */
#ifndef ND_API_H
#define ND_API_H
#include <stdint.h>

typedef void (*NdNapiRun)(const uint32_t* rec); /* core 0: draw one recorded command; rec = the payload, 8-byte aligned */
void nd_rec_napi(NdNapiRun run, const void* rec, unsigned units); /* nd_rec.c, core 1: rec holds units 8-byte units, 1 to 255 */
void nd_walk_state(const uint32_t* q, unsigned n); /* nd_walk.c, core 0: the typed op executor, as NDC_STATE runs it */

typedef uint16_t NdPso;
#define ND_PSO_MAX 256u

/* N64 state description: combine words (cw0, cw1, with the 0xfc byte), othermode L render mode value of gDPSetRenderMode,
 * othermode H bits the draw sets and their mask. */
typedef struct NdPsoN64 { uint32_t cw0, cw1, oml, omh, omh_m; } NdPsoN64;
NdPso nd_pso_n64(const NdPsoN64* d); /* core 1 */
const NdPsoN64* nd_pso_get(NdPso id); /* core 0 */

/* NATIVEAPI S4 (particles): one NDC_NAPI record for a run of effect draws, one instance per nd_parts_add. Each instance is m units of the
 * stream form (8 bytes each): pre state units, the NDO_CALL unit of its static flat list (a triangle list with one vertex load at slot 0),
 * post units. Core 0 (nd_parts_run) runs the pre units with the walker, draws the list natively (nd_part_draw) and runs the post units. */
typedef struct NdParts { unsigned n, pre, m, ok; const void* list; uint32_t u[2u + 2u * 252u]; } NdParts;
void nd_parts_begin(NdParts* b, unsigned pre, unsigned post); /* core 1 */
void nd_parts_add(NdParts* b, const uint32_t* units);        /* core 1: units = m units (2 words each) */
void nd_parts_end(NdParts* b);                               /* core 1: records the open run (no-op when empty) */
void nd_parts_run(const uint32_t* rec);                      /* nd_walk.c, core 0: the NDC_NAPI runner of a run */
void nd_parts_run_v(const uint32_t* rec);                    /* nd_walk.c, core 0: the runner of a run whose units are all typed setters (S4PDIR) */
int nd_part_draw(const uint32_t* list);                      /* nd_geom.c, core 0: one flat list natively; 0 = not drawn (run the list) */

/* NATIVEAPI A1 (NAPI2B): the shared direct-draw helpers. A direct executor (core 0) states a draw without the walker's op stream: the pso gives the
 * combine decision (made once, cold), the texture comes from a descriptor, the rectangle from the N64 rectangle words.
 * NdBindDesc: one texture window as a bind needs it (the facts ensure_tile_slow reads from nd_ws.td[n] and tst1). wrap: cms, cmt, masks, maskt (the
 * G_SETTILE words); sz: the SetTileSize window (10.2); x0, y0: the window origin in texels; stride in texels of the format. */
typedef struct NdBindDesc {
    const void* src; const void* pal;
    int pal_n; unsigned fmt, siz;
    int stride, x0, y0, w, h;
    unsigned wrap[4];
    int sz[4];
} NdBindDesc;
int nd_tex_bind_desc(const NdBindDesc* d, int n); /* nd_tex_set.c, core 0: bind d to tile n; 1 when the texture layer already held it */
int nd_tex_win_desc(NdBindDesc* o, const void* timg, int timg_stride, unsigned fmt, unsigned siz, const unsigned wrap[4], int ldx, int ldy, int ldw, int ldh,
                    int uls, int ult, int lrs, int lrt, const void* pal, int pal_n); /* nd_tex_set.c: tile 0 after a gDPLoadTextureTile (tex_win0); 0 = not valid */

typedef struct NdDec NdDec; /* nd_direct.c: the combine decision of one pso under the walker inputs */
const NdDec* nd_pso_decide(NdPso id);          /* core 0: the decision of pso id (made once per distinct inputs, cold); NULL when the walker state decides it */
int nd_direct_ready(void);                     /* core 0: 1 when the walker state lets a direct rectangle draw (main colour image, no shaded sprite, no off-screen tile 1); else -1 role, -2 colour image, -3 shaded sprite, -4 tile 1 target */
int nd_direct_bind(NdPso id, uint32_t prim, uint32_t env); /* core 0: the state of pso id for the next rectangles; 0 = not stated (nothing changed, run the old path) */
int nd_direct_tex(const NdBindDesc* d);        /* core 0: tile 0 = d; the walker's set_tilesize bookkeeping and the bind */
void nd_direct_rect(uint32_t w0, uint32_t w1, uint32_t hs, uint32_t hd); /* core 0: one texture rectangle of tile 0 (the G_TEXRECT words, 1 or 2 cycle) */
void nd_direct_end(void);                      /* core 0: the walker rebinds and resyncs at its next draw */
void nd_direct_fallback(unsigned why);        /* core 0, cold: a record the direct executor handed back to the old path (census line napihud) */
uint32_t nd_direct_prim(void);                 /* core 0: the walker's primitive colour */
uint32_t nd_direct_env(void);                  /* core 0: the walker's environment colour */

/* NATIVEAPI E5b (S5PQUAD): one fill rectangle with its primitive colour (S1FILL's record, plus the kind). x0..x1, y0..y1: the lower right is
 * exclusive. rgba: 0xRRGGBBAA. kind: 0 overlay translucent, 1 overlay opaque (CVG_DST_SAVE), 2 quad opaque (PM_CC_PRIM_FULL_ALPHA), 3 quad
 * translucent. nd_fill_run (nd_direct.c, core 0) states the combine and render mode of the kind and fills. */
typedef struct { int32_t x0, y0, x1, y1; uint32_t rgba, kind; } NdFillRec;
void nd_fill_run(const uint32_t* u); /* nd_direct.c, core 0: the NDC_NAPI runner of an NdFillRec */

#endif
