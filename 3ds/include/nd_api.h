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
int nd_part_draw(const uint32_t* list);                      /* nd_geom.c, core 0: one flat list natively; 0 = not drawn (run the list) */

#endif
