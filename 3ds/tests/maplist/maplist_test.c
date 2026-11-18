/* BARRICADE host test: the two map shape list walkers read u32 unit pairs (w0, w1 per unit) and follow CALL / VTX / TRI1 / END.
 * t_DetermineSphericalSize (barricade.c) and t_GetFirstTriangleNormal (GetFirstTriangleNormal.inc.c, from barricade.sh) are the real code.
 * Prints "barricade ok 2" or "barricade FAIL <what>". */
#include <stdio.h>
#include "common.h"
#include "nd_op.h"

void t_DetermineSphericalSize(const void*, s32*);
void t_GetFirstTriangleNormal(const void*, f32*, f32*, f32*);

#define VTXW0(n) (((uint32_t)ND_FS_VTX << 24) | ((n) << 12) | ((n) << 1))
#define CALLW0 ((uint32_t)ND_FS_CALL << 24)
#define ENDW0 ((uint32_t)ND_FS_END << 24)
#define TRI1W0 ((uint32_t)ND_FS_TRI1 << 24)
#define ADDR(p) ((uint32_t)(uintptr_t)(p))

static Vtx va, vb, vc;
static Vtx_t v4[4];
static uint32_t A[6], B[6], A2[6], B2[6];

static void setob(Vtx* p, s16 x, s16 y, s16 z) { p->v.ob[0] = x; p->v.ob[1] = y; p->v.ob[2] = z; }
static void setot(Vtx_t* p, s16 x, s16 y, s16 z) { p->ob[0] = x; p->ob[1] = y; p->ob[2] = z; }

int main(void) {
    s32 r = -1;
    f32 nx = 9.0f, ny = 9.0f, nz = 9.0f;

    /* case 1: radius over the VTX of A and of the called list B (max 12) */
    setob(&va, 3, 4, 0);
    setob(&vb, 6, 8, 0);
    setob(&vc, 0, 0, 12);
    B[0] = VTXW0(1); B[1] = ADDR(&vb);
    B[2] = VTXW0(1); B[3] = ADDR(&vc);
    B[4] = ENDW0;    B[5] = 0;
    A[0] = VTXW0(1); A[1] = ADDR(&va);
    A[2] = CALLW0;   A[3] = ADDR(B);
    A[4] = ENDW0;    A[5] = 0;
    t_DetermineSphericalSize((const void*)A, &r);
    if (r != 12) {
        printf("barricade FAIL radius %d\n", (int)r);
        return 1;
    }

    /* case 2: triangle normal of (0,0,0), (1,0,0), (0,0,1) read through a called list; expected (0, 1, 0) */
    setot(&v4[0], 0, 0, 0);
    setot(&v4[1], 1, 0, 0);
    setot(&v4[2], 0, 0, 1);
    setot(&v4[3], 5, 5, 5);
    B2[0] = TRI1W0; B2[1] = (0 << 16) | (2 << 8) | 4;
    B2[2] = ENDW0;  B2[3] = 0;
    A2[0] = VTXW0(4); A2[1] = ADDR(v4);
    A2[2] = CALLW0;   A2[3] = ADDR(B2);
    A2[4] = ENDW0;    A2[5] = 0;
    t_GetFirstTriangleNormal((const void*)A2, &nx, &ny, &nz);
    if (nx != 0.0f || ny != 1.0f || nz != 0.0f) {
        printf("barricade FAIL normal %f %f %f\n", nx, ny, nz);
        return 1;
    }

    printf("barricade ok 2\n");
    return 0;
}
