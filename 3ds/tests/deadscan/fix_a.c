/* DEADSCAN fixture: read by test_deadscan.py, never built. */

typedef unsigned int u32;
typedef struct Gfx Gfx;

void gSPVertex(Gfx *g, void *v, int n, int f);
void gDPSetRenderMode(Gfx *g, u32 a, u32 b);
void gDPPipeSync(Gfx *g);

static void inl_fn(Gfx *g) {
    gDPSetRenderMode(g, 1, 2);
}

void live_fn(Gfx *g) {
    gSPVertex(g, 0, 1, 0);
    gDPPipeSync(g);
    inl_fn(g);
}

void dead_fn(Gfx *g) {
    gSPVertex(g, 0, 1, 0);
    gDPSetRenderMode(g, 1, 2);
    gSPVertex(g, 0, 1, 0);
}
