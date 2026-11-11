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

#endif
