#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "pm_shader_shbin.h"
#include "pm_gpu.h"
#include "pm_gbi.h"
#include "pm_tex.h"
#include "pm_port.h"

typedef struct {
    float x, y, z, w;
    float u, v;
    float r, g, b, a;
} Vert;

enum { VERT_SLOTS = 32768 };
/* CONFIG2 stores bytes-per-vertex in 8 bits, and that value has to equal the
 * attribute size or the PICA freezes. 4+2+4 floats, no padding. */
_Static_assert(sizeof(Vert) == (4 + 2 + 4) * sizeof(float), "vertex stride");
static Vert* vert_base;
static Vert* verts;
static int nverts;
static int rect_2d;
static int drew;
static int gpu_ok;
/* 3D writes the depth buffer. A later texrect has to win even if the test stays on. */
static int depth_holds_3d;
static PrintConsole bottom_con;
static int z_on;
static int light_on;
static int combine;
static int batch_identity;
static int draw_3d;
static unsigned prim = 0xffffffffu;
static float game_mvp[4][4];
static float vp_sx = 160.f, vp_sy = 120.f, vp_tx = 160.f, vp_ty = 120.f;
static C3D_Mtx shader_3d;
/* N64 scissor, y down. The 400-wide target is the 320 game plus 40px bars. */
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static C3D_Mtx projection;
static int u_proj = -1;
static C3D_RenderTarget* target;
static shaderProgram_s prog;
static DVLB_s* dvlb;

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

/* F3DEX stores a row-vector multiply. The shader does dp4(row, v), which is
 * the transpose. Viewport then maps N64 clip/w into upright pixels, and
 * OrthoTilt turns those pixels into the rotated framebuffer. */
static void rebuild_3d(void) {
    C3D_Mtx game, view, tmp;
    int r;
    for (r = 0; r < 4; r++) {
        game.r[r].x = game_mvp[0][r];
        game.r[r].y = game_mvp[1][r];
        game.r[r].z = game_mvp[2][r];
        game.r[r].w = game_mvp[3][r];
    }
    Mtx_Zeros(&view);
    view.r[0].x = vp_sx;
    view.r[0].w = 40.f + vp_tx;
    view.r[1].y = vp_sy;
    view.r[1].w = 240.f - vp_ty;
    view.r[2].z = 0.4f;
    view.r[2].w = 0.5f;
    view.r[3].w = 1.f;
    Mtx_Multiply(&tmp, &view, &game);
    Mtx_Multiply(&shader_3d, &projection, &tmp);
}

static void flush(void) {
    if (nverts <= 0 || !gpu_ok) {
        nverts = 0;
        batch_identity = 0;
        return;
    }
    /* 2D batches are already pixels. 3D batches are object space. */
    if (u_proj >= 0)
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u_proj, draw_3d ? &shader_3d : &projection);
    /* Story pages and fades are submitted after the world. Clearing depth lets
     * those rectangles replace the mesh instead of failing the depth test. */
    if (rect_2d && depth_holds_3d && target) {
        C3D_RenderTargetClear(target, C3D_CLEAR_DEPTH, 0, 0);
        depth_holds_3d = 0;
    }
    if (draw_3d) depth_holds_3d = 1;
    (void)batch_identity;
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    int tex = (combine == 2) && pm_tex_bind();
    if (tex) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
    } else if (combine == 1) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        C3D_TexEnvColor(env, prim);
    } else {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    }
    /* Fill and texrects ignore the Z buffer. Startup turns G_ZBUFFER on
     * before the logo, and every 2D quad shares z, so a depth test keeps
     * only the first rectangle. */
    int z = z_on && !rect_2d;
    C3D_DepthTest(z ? true : false, GPU_GEQUAL, z ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    C3D_EarlyDepthTest(false, GPU_EARLYDEPTH_GREATER, 0);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    /* Target is 240x400, rotated so buffer Y is landscape X. Keep the 320-wide
     * game picture and whatever scissor the display list set inside it. */
    {
        int left = 240 - sci_y1;
        int right = 240 - sci_y0;
        int top = 40 + sci_x0;
        int bottom = 40 + sci_x1;
        if (left < 0) left = 0;
        if (right > 240) right = 240;
        if (top < 40) top = 40;
        if (bottom > 360) bottom = 360;
        if (right > left && bottom > top)
            C3D_SetScissor(GPU_SCISSOR_NORMAL, (u32)left, (u32)top, (u32)right, (u32)bottom);
        else
            C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 40, 240, 360);
    }
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
        GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    /* The GPU runs at FrameEnd. Each draw must keep its own vertices until then. */
    Vert* batch = verts;
    int count = nverts;
    GSPGPU_FlushDataCache(batch, sizeof(Vert) * (u32)count);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, batch, sizeof(Vert), 3, 0x210);
    C3D_DrawArrays(GPU_TRIANGLES, 0, count);
    verts = batch + count;
    nverts = 0;
    batch_identity = 0;
    drew = 1;
}

static void put4(float x, float y, float z, float w, float u, float v, unsigned c) {
    if (nverts >= 510) {
        int id = batch_identity;
        flush();
        batch_identity = id;
    }
    if (!verts || verts + nverts >= vert_base + VERT_SLOTS) return;
    Vert* p = &verts[nverts++];
    p->x = x;
    p->y = y;
    p->z = z;
    p->w = w;
    p->u = u;
    p->v = v;
    p->r = (float)((c >> 24) & 255) / 255.f;
    p->g = (float)((c >> 16) & 255) / 255.f;
    p->b = (float)((c >> 8) & 255) / 255.f;
    p->a = (float)(c & 255) / 255.f;
}

static void put(float x, float y, float z, float u, float v, unsigned c) {
    put4(x, y, z, 1.f, u, v, c);
}

void pm_gpu_init(void) {
    if (gpu_ok) return;
    gfxInitDefault();
    C3D_Init(0x400000);
    /* Width 240, height 400: that is the top screen's rotated framebuffer.
     * A 320-tall target copied into it skews every scanline. */
    target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    if (!target) {
        pm_log("render target failed\n");
        return;
    }
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    dvlb = DVLB_ParseFile((u32*)pm_shader_shbin, pm_shader_shbin_size);
    shaderProgramInit(&prog);
    shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    C3D_BindProgram(&prog);
    u_proj = shaderInstanceGetUniformLocation(prog.vertexShader, "projection");
    /* y grows up, matching the citro3d immediate example. 320 game pixels sit in the middle. */
    Mtx_OrthoTilt(&projection, 0.f, 400.f, 0.f, 240.f, 0.f, 1.f, true);
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 4);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(ai, 2, GPU_FLOAT, 4);
    vert_base = (Vert*)linearAlloc(sizeof(Vert) * VERT_SLOTS);
    verts = vert_base;
    ident(game_mvp);
    rebuild_3d();
    pm_tex_init();
    /* consoleGetDefault() stays the uninitialized template. The console printf
     * writes is the one passed to consoleInit. */
    consoleInit(GFX_BOTTOM, &bottom_con);
    setvbuf(stdout, NULL, _IONBF, 0);
    consoleSetWindow(&bottom_con, 1, 3, 40, 28);
    gpu_ok = vert_base != NULL;
    pm_log("gpu %s proj %d m %d %d", gpu_ok ? "ok" : "no vertex buffer", u_proj,
           (int)(projection.r[0].y * 1000.f), (int)(projection.r[0].w * 1000.f));
}

void pm_gpu_hud(const char* line0, const char* line1) {
    PrintConsole* c = &bottom_con;
    int cx, cy, wx, wy, ww, wh;
    if (!c->consoleInitialised) return;
    cx = c->cursorX;
    cy = c->cursorY;
    wx = c->windowX;
    wy = c->windowY;
    ww = c->windowWidth;
    wh = c->windowHeight;
    consoleSetWindow(c, 1, 1, 40, 2);
    c->cursorX = 1;
    c->cursorY = 1;
    printf("%-39.39s", line0 ? line0 : "");
    c->cursorX = 1;
    c->cursorY = 2;
    printf("%-39.39s", line1 ? line1 : "");
    fflush(stdout);
    consoleSetWindow(c, wx, wy, ww, wh);
    c->cursorX = cx;
    c->cursorY = cy;
}

void pm_gpu_begin(int clear) {
    drew = 0;
    depth_holds_3d = 0;
    nverts = 0;
    verts = vert_base;
    sci_x0 = 0;
    sci_y0 = 0;
    sci_x1 = 320;
    sci_y1 = 240;
    draw_3d = 0;
    if (!gpu_ok) return;
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    pm_tex_begin_frame();
    if (clear) C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
    C3D_FrameDrawOn(target);
}

void pm_gpu_end(void) {
    if (!gpu_ok) return;
    flush();
    pm_gbi_end_frame();
    /* Without this flag citro3d flushes the entire linear heap. That heap is
     * the 20MB port pool, so every frame was writing it back through the cache. */
    C3D_FrameEnd(GX_CMDLIST_FLUSH);
}

int pm_gpu_drew(void) { return drew; }

void pm_gpu_set_mvp(const float m[4][4], float sx, float sy, float tx, float ty) {
    if (sx == vp_sx && sy == vp_sy && tx == vp_tx && ty == vp_ty &&
        memcmp(game_mvp, m, sizeof(game_mvp)) == 0) return;
    if (draw_3d && nverts > 0) flush();
    memcpy(game_mvp, m, sizeof(game_mvp));
    vp_sx = sx;
    vp_sy = sy;
    vp_tx = tx;
    vp_ty = ty;
    rebuild_3d();
}

void pm_gpu_set_mode(int zbuffer, int lighting) {
    if (zbuffer == z_on && lighting == light_on) return;
    flush();
    z_on = zbuffer;
    light_on = lighting;
}

void pm_gpu_set_combine(int mode) {
    if (mode == combine) return;
    flush();
    combine = mode;
}

void pm_gpu_set_prim(unsigned rgba) { prim = rgba; }

void pm_gpu_set_scissor(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 320) x1 = 320;
    if (y1 > 240) y1 = 240;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    sci_x0 = x0;
    sci_y0 = y0;
    sci_x1 = x1;
    sci_y1 = y1;
}

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    if (draw_3d) {
        flush();
        draw_3d = 0;
    }
    put(x0, y0, z0, u0, v0, c0);
    put(x1, y1, z1, u1, v1, c1);
    put(x2, y2, z2, u2, v2, c2);
}

void pm_gpu_tri3d(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                  float x1, float y1, float z1, float u1, float v1, unsigned c1,
                  float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    if (!draw_3d) {
        flush();
        draw_3d = 1;
    }
    put(x0, y0, z0, u0, v0, c0);
    put(x1, y1, z1, u1, v1, c1);
    put(x2, y2, z2, u2, v2, c2);
}

void pm_gpu_vert_clip(float x, float y, float z, float w, float u, float v, unsigned c) {
    if (!draw_3d) {
        flush();
        draw_3d = 1;
    }
    put4(x, y, z, w, u, v, c);
}

/* Game pixels are y-down. The ortho matrix is y-up, and 320px is centered in 400. */
static float px(int x) { return 40.f + (float)x; }
static float py(int y) { return 240.f - (float)y; }

void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    flush();
    int saved = combine;
    combine = 0;
    rect_2d = 1;
    batch_identity = 1;
    float xa = px(x0);
    float xb = px(x1 + 1);
    float ya = py(y0);
    float yb = py(y1 + 1);
    pm_gpu_tri(xa, ya, 0.5f, 0, 0, rgba, xb, ya, 0.5f, 0, 0, rgba, xb, yb, 0.5f, 0, 0, rgba);
    pm_gpu_tri(xa, ya, 0.5f, 0, 0, rgba, xb, yb, 0.5f, 0, 0, rgba, xa, yb, 0.5f, 0, 0, rgba);
    flush();
    rect_2d = 0;
    combine = saved;
}

void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; float s = s0; s0 = s1; s1 = s; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; float s = t0; t0 = t1; t1 = s; }
    flush();
    int saved = combine;
    combine = 2;
    rect_2d = 1;
    batch_identity = 1;
    float xa = px(x0);
    float xb = px(x1 + 1);
    float ya = py(y0);
    float yb = py(y1 + 1);
    unsigned c = 0xffffffffu;
    pm_gpu_tri(xa, ya, 0.5f, s0, t0, c, xb, ya, 0.5f, s1, t0, c, xb, yb, 0.5f, s1, t1, c);
    pm_gpu_tri(xa, ya, 0.5f, s0, t0, c, xb, yb, 0.5f, s1, t1, c, xa, yb, 0.5f, s0, t1, c);
    flush();
    rect_2d = 0;
    combine = saved;
}
