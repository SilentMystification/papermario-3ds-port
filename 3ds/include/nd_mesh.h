/* MESHST: the draw of an animator mesh node (animator.c), built once per node: its units, each vertex load pointing at its own copy of
 * the vertices (vtxList with the xyz of the Vec3s array). Then 2 nu unit words, then the Vtx. 32 bytes keep the Vtx 8-aligned. */
#ifndef ND_MESH_H
#define ND_MESH_H
#include <stdint.h>
#include "nd_op.h"
#include "nd_gtab.h"
#include "nd_fs.h"

typedef struct NdMesh { NdNDev d; const void* base; uint32_t gen; uint32_t pad; } NdMesh;

static inline unsigned nd_mesh_nvtx(const uint32_t* u, unsigned nu) { unsigned n = 0; for (unsigned i = 0; i < nu; i++) if ((u[2u * i] >> 24) == ND_FS_VTX) n += (u[2u * i] >> 12) & 0xFFu; return n; }

static inline unsigned nd_mesh_size(const uint32_t* u, unsigned nu) { return (unsigned)sizeof(NdMesh) + 8u * nu + 16u * nd_mesh_nvtx(u, nu); }

static inline void nd_mesh_build(NdMesh* m, const uint32_t* u, unsigned nu, const Vtx* vl, int vso, const void* base) {
    uint32_t* uo = (uint32_t*)(void*)(m + 1); Vtx* vo = (Vtx*)(void*)(uo + 2u * nu); unsigned vi = 0;
    for (unsigned i = 0; i < nu; i++) {
        uint32_t w0 = u[2u * i], w1 = u[2u * i + 1u];
        if ((w0 >> 24) == ND_FS_VTX) {
            const unsigned n = (w0 >> 12) & 0xFFu;
            if (vl == NULL) w1 += (uint32_t)(vso + (int)vi) * 16u; /* animator.c:944 */
            else {
                uintptr_t p = (uintptr_t)w1 + (uintptr_t)(vso + (int)vi) * 6u; /* animator.c:950 */
                if (base != NULL) p = (p & 0xFFFFFFu) + (uintptr_t)base; /* animator.c:94-97 */
                const Vec3s* s = (const Vec3s*)p;
                for (unsigned j = 0; j < n; j++) { vo[vi + j] = vl[vi + j]; vo[vi + j].v.ob[0] = s[j].x; vo[vi + j].v.ob[1] = s[j].y; vo[vi + j].v.ob[2] = s[j].z; }
                w1 = (uint32_t)(uintptr_t)&vo[vi];
            }
            vi += n;
        }
        uo[2u * i] = w0; uo[2u * i + 1u] = w1;
    }
    m->d.list = m; m->d.fl = 0u; m->d.u = uo; m->d.nu = nu; m->d.ntri = 0u; m->base = base; m->gen = 0u; m->pad = 0u;
}
#endif
