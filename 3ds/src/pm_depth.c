/* F03: depth query. The decomp copies one z-buffer pixel into DepthCopyBuffer[id] with a texrect.
 * The port has no N64 z-buffer, so the GBI layer calls this instead: read one pixel of the PICA depth. */
#include "pm_depth.h"
#include "pm_gpu.h"
#include "pm_port.h"

extern unsigned short DepthCopyBuffer[16];
extern unsigned short* nuGfxZBuffer;

void pm_depth_query(const void* timg, int x, int id) {
    static int n;
    unsigned d24 = 0;
    int y = (int)(((unsigned)timg - (unsigned)nuGfxZBuffer) / 2u) / 320, ok;
    if (id < 0 || id > 15) return;
    ok = pm_gpu_depth_at(x, y, &d24);
    /* near = large d24 (GREATER test), so 0 near .. 1 far is 1 - d24/0xFFFFFF */
    DepthCopyBuffer[id] = ok ? pm_depth_encode((int)((1.0f - (float)d24 / 16777215.0f) * 32704.0f + 0.5f)) : 0;
    if (n++ < 8 || (n % 600) == 0) pm_log("F03 query n %d id %d x %d y %d ok %d d24 %06x word %04x", n, id, x, y, ok, d24, DepthCopyBuffer[id]);
}
