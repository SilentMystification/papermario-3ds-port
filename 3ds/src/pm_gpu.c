#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "pm_shader_shbin.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"

typedef struct {
    float x, y, z;
    float u, v;
    u8 r, g, b, a;
} Vert;

static Vert* verts;
static int nverts;
static int drew;
static int gpu_ok;
static int z_on;
static int light_on;
static int combine;
static int batch_identity;
static unsigned prim = 0xffffffffu;
static float mvp[4][4];
static C3D_RenderTarget* target;
static shaderProgram_s prog;
static DVLB_s* dvlb;

static void ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

static void upload(const float m[4][4]) {
    C3D_Mtx mtx;
    for (int r = 0; r < 4; r++) {
        mtx.r[r].x = m[r][0];
        mtx.r[r].y = m[r][1];
        mtx.r[r].z = m[r][2];
        mtx.r[r].w = m[r][3];
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, 0, &mtx);
}

static void flush(void) {
    if (nverts <= 0 || !gpu_ok) {
        nverts = 0;
        batch_identity = 0;
        return;
    }
    float id[4][4];
    if (batch_identity) {
        ident(id);
        upload(id);
    } else {
        upload(mvp);
    }
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
    C3D_DepthTest(z_on ? true : false, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
                   GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, verts, sizeof(Vert), 3, 0x210);
    C3D_DrawArrays(GPU_TRIANGLES, 0, nverts);
    nverts = 0;
    batch_identity = 0;
    drew = 1;
}

static void put(float x, float y, float z, float u, float v, unsigned c) {
    if (nverts >= 510) {
        int id = batch_identity;
        flush();
        batch_identity = id;
    }
    if (light_on) c = 0xffffffffu;
    Vert* p = &verts[nverts++];
    p->x = x;
    p->y = y;
    p->z = z;
    p->u = u;
    p->v = v;
    p->r = (u8)((c >> 24) & 255);
    p->g = (u8)((c >> 16) & 255);
    p->b = (u8)((c >> 8) & 255);
    p->a = (u8)(c & 255);
}

void pm_gpu_init(void) {
    if (gpu_ok) return;
    gfxInitDefault();
    C3D_Init(0x100000);
    target = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB565, GPU_RB_DEPTH16);
    if (!target) {
        pm_log("render target failed\n");
        return;
    }
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
        GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565) |
        GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
        GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    dvlb = DVLB_ParseFile((u32*)pm_shader_shbin, pm_shader_shbin_size);
    shaderProgramInit(&prog);
    shaderProgramSetVsh(&prog, &dvlb->DVLE[0]);
    C3D_BindProgram(&prog);
    C3D_AttrInfo* ai = C3D_GetAttrInfo();
    AttrInfo_Init(ai);
    AttrInfo_AddLoader(ai, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(ai, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(ai, 2, GPU_UNSIGNED_BYTE, 4);
    verts = (Vert*)linearAlloc(sizeof(Vert) * 512);
    ident(mvp);
    pm_tex_init();
    consoleInit(GFX_BOTTOM, NULL);
    gpu_ok = verts != NULL;
    pm_log("gpu %s 320x240", gpu_ok ? "ok" : "no vertex buffer");
}

void pm_gpu_begin(void) {
    drew = 0;
    nverts = 0;
    if (!gpu_ok) return;
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
    C3D_FrameDrawOn(target);
}

void pm_gpu_end(void) {
    if (!gpu_ok) return;
    flush();
    C3D_FrameEnd(0);
}

int pm_gpu_drew(void) { return drew; }

void pm_gpu_set_mvp(const float m[4][4]) {
    flush();
    memcpy(mvp, m, sizeof(mvp));
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

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2) {
    put(x0, y0, z0, u0, v0, c0);
    put(x1, y1, z1, u1, v1, c1);
    put(x2, y2, z2, u2, v2, c2);
}

void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    flush();
    batch_identity = 1;
    float xa = (float)x0 / 320.f * 2.f - 1.f;
    float xb = (float)(x1 + 1) / 320.f * 2.f - 1.f;
    float ya = 1.f - (float)y0 / 240.f * 2.f;
    float yb = 1.f - (float)(y1 + 1) / 240.f * 2.f;
    pm_gpu_tri(xa, ya, 0, 0, 0, rgba, xb, ya, 0, 0, 0, rgba, xb, yb, 0, 0, 0, rgba);
    pm_gpu_tri(xa, ya, 0, 0, 0, rgba, xb, yb, 0, 0, 0, rgba, xa, yb, 0, 0, 0, rgba);
    flush();
}

void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; float s = s0; s0 = s1; s1 = s; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; float s = t0; t0 = t1; t1 = s; }
    flush();
    int saved = combine;
    combine = 2;
    batch_identity = 1;
    float xa = (float)x0 / 320.f * 2.f - 1.f;
    float xb = (float)(x1 + 1) / 320.f * 2.f - 1.f;
    float ya = 1.f - (float)y0 / 240.f * 2.f;
    float yb = 1.f - (float)(y1 + 1) / 240.f * 2.f;
    unsigned c = 0xffffffffu;
    pm_gpu_tri(xa, ya, 0, s0, t0, c, xb, ya, 0, s1, t0, c, xb, yb, 0, s1, t1, c);
    pm_gpu_tri(xa, ya, 0, s0, t0, c, xb, yb, 0, s1, t1, c, xa, yb, 0, s0, t1, c);
    flush();
    combine = saved;
}
