#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
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
static PrintConsole bottom_con;
static int z_on;
static int light_on;
/* RDP render mode: depth compare, depth update, alpha blend, alpha cut-off (-1 none), decal. */
static int r_zcmp = 1, r_zupd = 1, r_blend = 1, r_atest = -1, r_decal;
static int combine;
int pm_debug_value(const char* key, char* out, unsigned n);
static int dview = -1; /* V02 depth view mode, see dv_color */
static unsigned dv_draw;
static int rect_z; /* X01: the 2D rectangle in flight tests depth (fill rect with G_ZS_PRIM) */
static int r_nocol; /* S25: the blender gives back the frame buffer colour: depth only */
static PmPlan plan; /* S25: combine mode 8 */
static int env_hi = 1; /* texenv stages configured so far (stage 0 is always reset) */
static unsigned env_col = 0xffffffffu; /* combine mode 5: the ENV colour */
static unsigned cconst;
static int rgb_scale = 1;
static int batch_identity;
static int draw_3d;
static unsigned prim = 0xffffffffu;
static float game_mvp[4][4];
static float vp_sx = 160.f, vp_sy = 120.f, vp_tx = 160.f, vp_ty = 120.f;
static C3D_Mtx shader_3d;
/* N64 scissor, y down. The 400-wide target is the 320 game plus 40px bars. */
static int sci_x0, sci_y0, sci_x1 = 320, sci_y1 = 240;
static C3D_Mtx projection;
static int u_proj = -1, u_uvx = -1;
static float t1_s[4] = { 1.f, 1.f, 0.f, 0.f };
static int aux_bound;
static C3D_RenderTarget* target;
/* H02 GPU limit stats, maxima since the last `mem` log line. Verts and command buffer overflow drop work without a message. */
static unsigned draws_frame, draws_max, verts_max, vert_drops, frames_seen;
static float cmd_max;
static shaderProgram_s prog;
static DVLB_s* dvlb;

static int off_active; /* V05: drawing goes to an off-screen image (see pm_gpu_off_begin) */
static C3D_Mtx off_proj;

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
    /* V05: an off-screen pass is not rotated and has no 40 pixel margin: GPU y is the game y (y runs down) */
    view.r[0].x = vp_sx;
    view.r[0].w = (off_active ? 0.f : 40.f) + vp_tx;
    view.r[1].y = off_active ? -vp_sy : vp_sy;
    view.r[1].w = off_active ? vp_ty : 240.f - vp_ty;
    /* Clip z/w is -1 at the near plane and +1 at the far plane. Citro's
     * ortho then wants near at -1 and far at 0, which is the value GEQUAL
     * keeps. The opposite sign stores the far wall as the nearer depth, so
     * the house covers the yard. */
    view.r[2].z = 0.5f;
    view.r[2].w = 0.5f;
    view.r[3].w = 1.f;
    Mtx_Multiply(&tmp, &view, &game);
    Mtx_Multiply(&shader_3d, off_active ? &off_proj : &projection, &tmp);
}

/* The PICA constant colour register is 0xAABBGGRR. The port keeps colours as 0xRRGGBBAA. */
static unsigned abgr(unsigned c) {
    return ((c & 255u) << 24) | (((c >> 8) & 255u) << 16) | (((c >> 16) & 255u) << 8) | (c >> 24);
}

static GPU_TEVSRC tev_src(int s) {
    switch (s) {
    case PM_S_TEX0: return GPU_TEXTURE0;
    case PM_S_TEX1: return GPU_TEXTURE1;
    case PM_S_VTX: return GPU_PRIMARY_COLOR;
    case PM_S_CONST: return GPU_CONSTANT;
    case PM_S_PREV: return GPU_PREVIOUS;
    default: return GPU_PREVIOUS_BUFFER;
    }
}
static GPU_TEVOP_RGB tev_op_rgb(int o) {
    return o == PM_O_C ? GPU_TEVOP_RGB_SRC_COLOR : o == PM_O_1MC ? GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR :
           o == PM_O_A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA;
}
static GPU_TEVOP_A tev_op_a(int o) { return (o == PM_O_1MA || o == PM_O_1MC) ? GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA : GPU_TEVOP_A_SRC_ALPHA; }
static GPU_COMBINEFUNC tev_func(int f) {
    switch (f) {
    case PM_F_REPLACE: return GPU_REPLACE;
    case PM_F_MODULATE: return GPU_MODULATE;
    case PM_F_ADD: return GPU_ADD;
    case PM_F_SUBTRACT: return GPU_SUBTRACT;
    case PM_F_INTERP: return GPU_INTERPOLATE;
    default: return GPU_MULTIPLY_ADD;
    }
}

/* V05: an off-screen colour image. draw_box renders the contents of a rotating window (pause pages, file select panels)
 * into a game buffer with the color image set to it, then draws that buffer as strips through the rotation matrix.
 * Such a pass renders into a PICA render target that is a texture (VRAM), and pm_tex samples it for the strips. */
enum { OFF_N = 4 };
/* X02: w x h is the size of the slot (64 x 64 for a game image up to 64 wide, else PM_OFF_W x PM_OFF_H). siz is the game pixel size
 * (3 = 32 bit, 2 = 16 bit, 1 = 8 bit). The same memory can be a 32 bit image and an 8 bit image in one frame: two slots. */
static struct { C3D_Tex tex; C3D_RenderTarget* rt; const void* base; int w, h, siz; } offs[OFF_N];
static int off_used, off_cur, off_fail, off_w = PM_OFF_W, off_h = PM_OFF_H, off_i8;
extern unsigned pm_frame_count(void);
static int r_mix; static unsigned r_mixcol; /* V06: see pm_gpu_set_mix */
static int r_replace; /* off-screen fill: the pixel is written, not blended */

static void flush(void) {
    if (nverts <= 0 || !gpu_ok) {
        nverts = 0;
        batch_identity = 0;
        return;
    }
    if (r_nocol && !(z_on && !rect_2d && r_zupd)) {
        nverts = 0;
        batch_identity = 0;
        return;
    }
    /* H03: the texture of this batch failed to load (cache full). Skip the batch, a white quad is worse than a missing one. */
    if (combine >= 2 && pm_tex_failed() && (combine != 8 || plan.uses_tex || plan.uses_tex1)) {
        nverts = 0;
        batch_identity = 0;
        return;
    }
    PH_PUSH(PH_GPU);
    /* 2D batches are already pixels. 3D batches are object space. */
    if (u_proj >= 0)
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, u_proj, draw_3d ? &shader_3d : off_active ? &off_proj : &projection); /* V05: shader_3d follows off_active, see rebuild_3d */
    /* Rectangles skip the depth test, so a story page already covers the mesh.
     * Wiping the buffer here let the next 3D batch ignore the house. */
    (void)batch_identity;
    for (int i = 1; i < env_hi; i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
    env_hi = 1;
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    int tex = (combine >= 2) && pm_tex_bind();
    int tex1 = combine == 8 && plan.uses_tex1 && pm_tex_bind_aux();
    if (tex1 && u_uvx >= 0) C3D_FVUnifSet(GPU_VERTEX_SHADER, u_uvx, t1_s[0], t1_s[1], t1_s[2], t1_s[3]);
    /* Unit 1 stays bound to the last TEXEL1 texture: C3D_TexBind(1, NULL) crashes. No stage reads it. */
    if (combine == 8 && (!plan.uses_tex || tex) && (!plan.uses_tex1 || tex1)) {
        /* S25: general combiner. A stage reads the texture, the vertex colour, its constant, the previous
         * stage or the buffer colour. The buffer is never updated by a stage, so it holds one constant. */
        C3D_TexEnvBufUpdate(C3D_Both, 0);
        C3D_TexEnvBufColor(abgr(plan.buf));
        for (int i = 0; i < plan.n && i < 6; i++) {
            C3D_TexEnv* e = C3D_GetTexEnv(i);
            const PmTevStage* st = &plan.st[i];
            C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e, C3D_RGB, tev_src(st->g[0].src[0]), tev_src(st->g[0].src[1]), tev_src(st->g[0].src[2]));
            C3D_TexEnvOpRgb(e, tev_op_rgb(st->g[0].op[0]), tev_op_rgb(st->g[0].op[1]), tev_op_rgb(st->g[0].op[2]));
            C3D_TexEnvFunc(e, C3D_RGB, tev_func(st->g[0].func));
            C3D_TexEnvSrc(e, C3D_Alpha, tev_src(st->g[1].src[0]), tev_src(st->g[1].src[1]), tev_src(st->g[1].src[2]));
            C3D_TexEnvOpAlpha(e, tev_op_a(st->g[1].op[0]), tev_op_a(st->g[1].op[1]), tev_op_a(st->g[1].op[2]));
            C3D_TexEnvFunc(e, C3D_Alpha, tev_func(st->g[1].func));
            C3D_TexEnvColor(e, abgr(st->constant));
        }
        env_hi = plan.n;
    } else if (combine == 8) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    } else if (tex && combine == 6) {
        /* (TEXEL - PRIM) * PRIM_ALPHA + PRIM: the texture fades to the primitive colour (pause screen
         * background, darkened by gray 40 at alpha 128). Alpha = texel alpha. */
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_TEXTURE0, GPU_CONSTANT);
        C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        C3D_TexEnvColor(env, abgr(prim));
    } else if (tex && (combine == 5 || combine == 7)) {
        /* (PRIM - ENV) * TEXEL + ENV, alpha = texel alpha. ENV rides in the vertex colour. */
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_TEXTURE0);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
        if (combine == 7) {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvColor(env, abgr(prim | 0xffu));
        } else {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvColor(env, abgr(prim));
        }
    } else if (tex && combine == 4) {
        /* Flat colour, alpha = texture alpha times the constant's alpha. */
        C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, GPU_CONSTANT, GPU_CONSTANT);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, GPU_CONSTANT);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
        C3D_TexEnvColor(env, abgr(cconst));
    } else if (tex) {
        C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_MODULATE);
        if (combine == 3) {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        } else {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
        }
        if (rgb_scale == 2) C3D_TexEnvScale(env, C3D_RGB, GPU_TEVSCALE_2);
    } else if (combine == 1) {
        C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        C3D_TexEnvColor(env, abgr(prim));
    } else {
        C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    }
    if (r_mix && draw_3d && !rect_2d && env_hi < 6 && !dview) {
        /* V06: colour = mix * alpha + previous * (1 - alpha), alpha = the constant's alpha. The alpha channel passes. */
        C3D_TexEnv* e = C3D_GetTexEnv(env_hi);
        C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e, C3D_RGB, GPU_CONSTANT, GPU_PREVIOUS, GPU_CONSTANT);
        C3D_TexEnvOpRgb(e, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e, C3D_RGB, GPU_INTERPOLATE);
        C3D_TexEnvSrc(e, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
        C3D_TexEnvFunc(e, C3D_Alpha, GPU_REPLACE);
        C3D_TexEnvColor(e, abgr(r_mixcol));
        env_hi++;
    }
    if (off_active && off_i8 && env_hi < 6) {
        /* X02: an 8 bit intensity image reads as I in all four channels. The pixel written is (R, R, R, R). */
        C3D_TexEnv* e = C3D_GetTexEnv(env_hi);
        C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
        C3D_TexEnvOpAlpha(e, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R, GPU_TEVOP_A_SRC_R);
        C3D_TexEnvFunc(e, C3D_Both, GPU_REPLACE);
        env_hi++;
    }
    /* Fill and texrects ignore the Z buffer. Startup turns G_ZBUFFER on
     * before the logo, and every 2D quad shares z, so a depth test keeps
     * only the first rectangle. */
    if (dview && draw_3d && !rect_2d) { /* V02 depth view: vertex colour only, alpha from the texture */
        for (int i = 0; i < 6; i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
        env_hi = 6;
        env = C3D_GetTexEnv(0);
        C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
        if (tex) {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
        } else {
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
        }
    }
    dv_draw++;
    int z = z_on && (!rect_2d || rect_z) && !off_active;
    /* Citro's default. Near is -1 and far is 0, and GREATER keeps the near fragment. A decal
     * sits in the same plane as its base, so equal depth has to pass. */
    if (z && r_zcmp)
        C3D_DepthTest(true, r_decal ? GPU_GEQUAL : GPU_GREATER, r_nocol ? GPU_WRITE_DEPTH : r_zupd ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    else if (z && r_zupd)
        C3D_DepthTest(true, GPU_ALWAYS, r_nocol ? GPU_WRITE_DEPTH : GPU_WRITE_ALL);
    else
        C3D_DepthTest(false, GPU_GREATER, GPU_WRITE_COLOR);
    /* A decal lies in the plane of its base surface. Move it a little toward the eye. */
    C3D_DepthMap(true, -1.0f, r_decal ? 0.001f : 0.0f);
    C3D_EarlyDepthTest(false, GPU_EARLYDEPTH_GREATER, 0);
    if (r_atest >= 0) C3D_AlphaTest(true, GPU_GREATER, r_atest);
    else C3D_AlphaTest(false, GPU_ALWAYS, 0);
    /* Target is 240x400, rotated so buffer Y is landscape X. Keep the 320-wide
     * game picture and whatever scissor the display list set inside it. */
    if (off_active) {
        /* not rotated: x is the game x. GPU y is the game y: row 0 of the image is the last memory row, where v = 0 samples */
        int x1 = sci_x1 > off_w ? off_w : sci_x1, y1 = sci_y1 > off_h ? off_h : sci_y1;
        if (x1 > sci_x0 && y1 > sci_y0)
            C3D_SetScissor(GPU_SCISSOR_NORMAL, (u32)sci_x0, (u32)sci_y0, (u32)x1, (u32)y1);
        else
            C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, off_w, off_h);
    } else {
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
    if (off_active && (r_replace || off_i8))
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    else if (off_active) /* alpha accumulates as source over destination: a covered pixel becomes opaque */
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
            GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
    else if (r_blend || rect_2d)
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    else
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    /* The GPU runs at FrameEnd. Each draw must keep its own vertices until then. */
    Vert* batch = verts;
    int count = nverts;
    PH_PUSH(PH_GDRAW);
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, batch, sizeof(Vert), 3, 0x210);
    C3D_DrawArrays(GPU_TRIANGLES, 0, count);
    PH_POP();
    verts = batch + count;
    /* 16-byte aligned start for the next batch (a vertex is 40 bytes, so every other batch start moves one slot). */
    if ((unsigned)(uintptr_t)verts & 15u) verts++;
    draws_frame++;
    PH_ADD(C_DRAW, 1);
    PH_ADD(C_VTXSLOT, count);
    PH_POP();
    nverts = 0;
    batch_identity = 0;
    drew = 1;
}

/* V02 debug switch depthview=<1|2|3>: 3D fragments get a depth or draw index colour instead of the combiner colour
 * (texture alpha kept, so cut-outs show). 1: depth, red near to blue far. 2: one colour per draw (flush) index.
 * 3: depth bands (repeating, 64 per range) to see depth order faults of near-equal surfaces. */
static unsigned dv_color(float z, float w, unsigned c) {
    float r, g, b, d;
    if (dview == 2) {
        unsigned h = dv_draw * 2654435761u;
        r = (float)((h >> 8) & 255) / 255.f; g = (float)((h >> 16) & 255) / 255.f; b = (float)((h >> 24) & 255) / 255.f;
    } else {
        d = (w > 0.f ? z / w : 0.f) * 0.5f + 0.5f;
        if (d < 0.f) d = 0.f;
        if (d > 1.f) d = 1.f;
        if (dview == 3) {
            float f = d * 64.f;
            f -= (float)(int)f;
            r = f; g = 1.f - f; b = d;
        } else {
            r = d; g = 1.f - (d < 0.5f ? 1.f - 2.f * d : 2.f * d - 1.f); b = 1.f - d;
        }
    }
    return ((unsigned)(r * 255.f) << 24) | ((unsigned)(g * 255.f) << 16) | ((unsigned)(b * 255.f) << 8) | (c & 255u);
}

static void put4(float x, float y, float z, float w, float u, float v, unsigned c) {
    if (dview < 0) {
        char vv[8];
        dview = pm_debug_value("depthview", vv, sizeof vv) ? atoi(vv) : 0;
    }
    if (dview && draw_3d && !rect_2d) c = dv_color(z, w, c);
    if (nverts >= 510) {
        int id = batch_identity;
        flush();
        batch_identity = id;
    }
    if (!verts || verts + nverts >= vert_base + VERT_SLOTS) {
        vert_drops++;
        return;
    }
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
    if (!C3D_Init(0x400000)) {
        pm_con_lock();
        consoleInit(GFX_BOTTOM, &bottom_con);
        printf("C3D_Init failed. No graphics.");
        pm_con_unlock();
        pm_log("C3D_Init failed (command buffer 4MB), no graphics");
        return;
    }
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
    u_uvx = shaderInstanceGetUniformLocation(prog.vertexShader, "uvx");
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
    pm_con_lock();
    consoleInit(GFX_BOTTOM, &bottom_con);
    setvbuf(stdout, NULL, _IONBF, 0);
    consoleSetWindow(&bottom_con, 1, 3, 40, 28);
    pm_con_unlock();
    gpu_ok = vert_base != NULL;
    if (!vert_base) pm_log("linearAlloc for %u vertex slots failed, nothing will draw", (unsigned)VERT_SLOTS);
    pm_log("gpu %s proj %d m %d %d", gpu_ok ? "ok" : "no vertex buffer", u_proj,
           (int)(projection.r[0].y * 1000.f), (int)(projection.r[0].w * 1000.f));
}

void pm_gpu_hud(const char* line0, const char* line1) {
    PrintConsole* c = &bottom_con;
    int cx, cy, wx, wy, ww, wh;
    if (!c->consoleInitialised) return;
    /* The log writer thread prints on this console. Skip the HUD update when it is busy: the game thread never waits for it. */
    if (pm_con_trylock() != 0) return;
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
    pm_con_unlock();
}

/* shots switch: dump the last finished frame (rotated 240x400 buffer, tiled RGBA8) as a PPM. */
int pm_debug_value(const char* key, char* out, unsigned n);
static int want_clear; /* X01: the clear runs at frame end, and only for a frame the game drew */
static int shot_burst;
void pm_gpu_shot_burst(int n) { shot_burst = n; }
static void dump_shot(void) {
    static unsigned n, frames;
    char path[64];
    FILE* f;
    unsigned char* row;
    const unsigned char* src;
    int x, y;
    static int skip = -1;
    if (!pm_debug_has("shots")) return;
    if (skip < 0) { /* shotskip=<n>: no shot in the first n frames (a later scene, such as the second intro after the demo) */
        char v[16];
        skip = pm_debug_value("shotskip", v, sizeof v) ? atoi(v) : 0;
    }
    {   /* X01: shotwin=<first frame>:<count> takes one shot per game frame (frame = pm_frame_count) */
        static int w0 = -2, w1;
        extern unsigned pm_frame_count(void);
        if (w0 == -2) {
            char v[24];
            w0 = -1;
            if (pm_debug_value("shotwin", v, sizeof v) && sscanf(v, "%d:%d", &w0, &w1) == 2) w1 += w0;
            else w0 = -1;
        }
        if (shot_burst > 0) {
            shot_burst--;
            pm_log("shot %u frame %u", n, pm_frame_count());
        } else if (w0 >= 0) {
            unsigned fc = pm_frame_count();
            if ((int)fc < w0 || (int)fc >= w1) return;
            pm_log("shot %u frame %u", n, fc);
        } else if (frames++ < (unsigned)skip || (frames % 30) != 0 || n >= 200) return;
    }
    src = (const unsigned char*)target->frameBuf.colorBuf;
    if (!src) return;
    mkdir("shots", 0777);
    snprintf(path, sizeof path, "shots/%04u.ppm", n++);
    f = fopen(path, "wb");
    if (!f) return;
    fputs("P6 240 400 255\n", f);
    row = (unsigned char*)malloc(240 * 3);
    for (y = 0; y < 400; y++) {
        for (x = 0; x < 240; x++) {
            unsigned mx = x & 7u, my = y & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                         ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((unsigned)(y >> 3) * 30u + (unsigned)(x >> 3)) * 64u + m) * 4u);
            row[x * 3] = p[3];
            row[x * 3 + 1] = p[2];
            row[x * 3 + 2] = p[1];
        }
        fwrite(row, 1, 240 * 3, f);
    }
    free(row);
    fclose(f);
}

/* D03: the last finished frame as N64 RGBA5551 (native u16, alpha bit set), 320x240, for game code that
 * reads the frame buffer. Reads the PICA color buffer (tiled RGBA8, 240 wide x 400 tall, the screen rotated;
 * game x is the long axis shifted by 40, game y runs backwards on the short axis). Costs one pass over 76800
 * pixels of VRAM, so it runs only when the game asks (pause screen, previous-frame draws). */
void pm_gpu_read_frame(unsigned short* dst) {
    const unsigned char* src;
    int x, y;
    if (!target || !dst) return;
    src = (const unsigned char*)target->frameBuf.colorBuf;
    if (!src) return;
    PH_PUSH(PH_FBREAD);
    for (y = 0; y < 240; y++) {
        unsigned fx = (unsigned)(239 - y);
        for (x = 0; x < 320; x++) {
            unsigned fy = (unsigned)(40 + x);
            unsigned mx = fx & 7u, my = fy & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) |
                         ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((fy >> 3) * 30u + (fx >> 3)) * 64u + m) * 4u);
            dst[y * 320 + x] = (unsigned short)(((p[3] >> 3) << 11) | ((p[2] >> 3) << 6) | ((p[1] >> 3) << 1) | 1u);
        }
    }
    PH_POP();
}

/* V05: start (or go back to) the off-screen pass for the colour image at base, width stride pixels. Returns 1 if
 * drawing goes there now. Each base gets one image for the frame (cleared on first use). */
int pm_gpu_off_begin(const void* base, int stride, int siz) {
    int i, w, h;
    if (!gpu_ok || off_fail || stride < 1 || stride > PM_OFF_W) return 0;
    if (off_active && offs[off_cur].base == base && offs[off_cur].siz == siz) return 1;
    flush();
    for (i = 0; i < off_used; i++) if (offs[i].base == base && offs[i].siz == siz) break;
    w = stride <= 64 ? 64 : PM_OFF_W; /* X02: the darkness stencil is 64 x 64: 16 KB of VRAM, not 512 KB */
    h = stride <= 64 ? 64 : PM_OFF_H;
    if (i == off_used) {
        if (off_used >= OFF_N) {
            static int once;
            if (!once++) pm_log("V05: more than %d off-screen images in one frame, the rest draw on the screen", OFF_N);
            return 0;
        }
        if (offs[i].rt && (offs[i].w != w || offs[i].h != h)) { /* the slot has the other size: make it again */
            C3D_RenderTargetDelete(offs[i].rt);
            C3D_TexDelete(&offs[i].tex);
            offs[i].rt = NULL;
        }
        if (!offs[i].rt) {
            if (!C3D_TexInitVRAM(&offs[i].tex, w, h, GPU_RGBA8)) {
                pm_log("V05: off-screen image %d: no VRAM, off-screen passes stay on the screen", i);
                off_fail = 1;
                return 0;
            }
            C3D_TexSetFilter(&offs[i].tex, GPU_LINEAR, GPU_NEAREST);
            C3D_TexSetWrap(&offs[i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
            offs[i].rt = C3D_RenderTargetCreateFromTex(&offs[i].tex, GPU_TEXFACE_2D, 0, -1);
            if (!offs[i].rt) {
                pm_log("V05: off-screen image %d: render target failed", i);
                C3D_TexDelete(&offs[i].tex);
                off_fail = 1;
                return 0;
            }
            offs[i].w = w;
            offs[i].h = h;
        }
        C3D_RenderTargetClear(offs[i].rt, C3D_CLEAR_COLOR, 0, 0);
        offs[i].base = base;
        offs[i].siz = siz;
        pm_tex_set_offscreen(i, base, (unsigned)stride, &offs[i].tex, siz, offs[i].w, offs[i].h);
        if (w == 64) { static int n; if (n++ < 6) pm_log("X02: darkness draw to 64x64 slot %d size %d bits %d addr %p", i, stride, siz == 3 ? 32 : siz == 2 ? 16 : 8, base); }
        off_used++;
    }
    off_cur = i;
    off_w = offs[i].w;
    off_h = offs[i].h;
    off_i8 = siz == 1;
    Mtx_Ortho(&off_proj, 0.f, (float)off_w, 0.f, (float)off_h, 0.f, 1.f, true);
    off_active = 1;
    rebuild_3d();
    C3D_FrameDrawOn(offs[i].rt);
    return 1;
}

/* V05: back to the screen. */
void pm_gpu_off_end(void) {
    if (!off_active) return;
    flush();
    off_active = 0;
    off_i8 = 0;
    rebuild_3d();
    C3D_FrameDrawOn(target);
}

void pm_gpu_set_replace(int on) { r_replace = on; }

/* V05 debug switch offdump: the first off-screen image of the last frame, as shots/off<frame>.ppm (memory row 0 on top) */
static void off_dump(void) {
    static int on = -1, n, from;
    int k;
    char v[16];
    FILE* f;
    unsigned char* row;
    const unsigned char* src;
    if (on < 0) { from = pm_debug_value("offdump", v, sizeof v) ? atoi(v) : 0; on = from || pm_debug_has("offdump"); } /* offdump=<first frame> */
    if (!on || !off_used || n >= 12 || (int)pm_frame_count() < from) return;
    /* X02: every slot of the frame, 64 x 64 slots too: off<frame>_<slot>.ppm */
  for (k = 0; k < off_used && n < 12; k++) {
    src = (const unsigned char*)offs[k].tex.data;
    mkdir("shots", 0777);
    char path[48];
    snprintf(path, sizeof path, "shots/off%03u_%d.ppm", pm_frame_count(), k);
    n++;
    f = fopen(path, "wb");
    if (!f) continue;
    fprintf(f, "P6 %d %d 255\n", offs[k].w, offs[k].h);
    row = (unsigned char*)malloc(offs[k].w * 3);
    for (int y = 0; y < offs[k].h; y++) {
        for (int x = 0; x < offs[k].w; x++) {
            unsigned mx = x & 7u, my = y & 7u;
            unsigned m = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) | ((mx & 4u) << 2) | ((my & 4u) << 3);
            const unsigned char* p = src + ((((unsigned)(y >> 3) * (offs[k].w / 8) + (unsigned)(x >> 3)) * 64u + m) * 4u);
            unsigned a = p[0];
            row[x * 3] = (unsigned char)(p[3] * a / 255); row[x * 3 + 1] = (unsigned char)(p[2] * a / 255); row[x * 3 + 2] = (unsigned char)(p[1] * a / 255);
            if (a == 0) row[x * 3] = row[x * 3 + 1] = row[x * 3 + 2] = 40;
        }
        fwrite(row, 1, offs[k].w * 3, f);
    }
    free(row);
    fclose(f);
  }
}

void pm_gpu_begin(int clear) {
    off_dump();
    off_used = 0;
    off_active = 0;
    off_i8 = 0;
    pm_tex_clear_offscreen();
    dv_draw = 0;
    draws_frame = 0;
    drew = 0;
    nverts = 0;
    verts = vert_base;
    sci_x0 = 0;
    sci_y0 = 0;
    sci_x1 = 320;
    sci_y1 = 240;
    draw_3d = 0;
    if (!gpu_ok) return;
    PH_PUSH(PH_FBEGIN); /* C3D_FRAME_SYNCDRAW: waits for the previous frame on the GPU */
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    PH_POP();
    dump_shot();
    PH_PUSH(PH_TEX);
    pm_tex_begin_frame();
    PH_POP();
    PH_PUSH(PH_GPU);
    want_clear = clear;
    C3D_FrameDrawOn(target);
    PH_POP();
}

void pm_gpu_end(void) {
    if (!gpu_ok) return;
    flush();
    if (off_active) pm_gpu_off_end();
    /* X01: a frame the game did not draw (it skips gfx_draw_frame while the pause screen takes the picture) keeps the last
     * picture on screen, as the N64 does when no task swaps the buffer. A clear here would show black for that frame. The clear
     * is a GX memory fill that runs before the command list queued by C3D_FrameEnd, so it can come after the draw calls. */
    if (want_clear && drew) C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0, 0);
    /* PO1: the GPU reads the vertex buffer when C3D_FrameEnd runs the command list. One flush of the
     * whole range written this frame replaces one GSPGPU call per draw. Each frame starts at vert_base
     * again, and FrameBegin(SYNCDRAW) has waited for the GPU, so no draw of this frame overlaps the
     * writes. */
    if (verts && verts > vert_base) {
        PH_PUSH(PH_GCACHE);
        GSPGPU_FlushDataCache(vert_base, (u32)((uintptr_t)verts - (uintptr_t)vert_base));
        PH_POP();
    }
    {
        unsigned used = verts ? (unsigned)(verts - vert_base) : 0;
        float cmd = C3D_GetCmdBufUsage();
        static unsigned warned;
        frames_seen++;
        if (draws_frame > draws_max) draws_max = draws_frame;
        if (used > verts_max) verts_max = used;
        if (cmd > cmd_max) cmd_max = cmd;
        if ((used * 10u > VERT_SLOTS * 9u || cmd > 0.9f || vert_drops) && (warned < 5 || frames_seen % 600 == 0)) {
            warned++;
            pm_log("GPU LIMIT NEAR: vertex slots %u of %u (dropped %u), command buffer %d%%, draws %u", used,
                   (unsigned)VERT_SLOTS, vert_drops, (int)(cmd * 100.f), draws_frame);
        }
    }
    pm_gbi_end_frame();
    /* Without this flag citro3d flushes the entire linear heap. That heap is
     * the 20MB port pool, so every frame was writing it back through the cache. */
    PH_PUSH(PH_FEND);
    C3D_FrameEnd(GX_CMDLIST_FLUSH);
    PH_POP();
}

int pm_gpu_drew(void) { return drew; }

void pm_gpu_log_stats(const char* tag) {
    pm_log("mem %s gpu max per frame: draws %u, vertex slots %u of %u, command buffer %d%%, vertex drops (total) %u", tag, draws_max,
           verts_max, (unsigned)VERT_SLOTS, (int)(cmd_max * 100.f), vert_drops);
    draws_max = verts_max = 0;
    cmd_max = 0.f;
}

/* Fatal boot problem (no ROM, wrong ROM): show it on the bottom screen and wait for START. Used before the GPU is up. */
void pm_fatal_screen(const char* msg) {
    pm_log("FATAL: %s", msg);
    pm_con_lock();
    if (!bottom_con.consoleInitialised) {
        gfxInitDefault();
        consoleInit(GFX_BOTTOM, &bottom_con);
    } else {
        consoleSelect(&bottom_con);
        consoleSetWindow(&bottom_con, 0, 0, 40, 30);
    }
    consoleClear();
    printf("Paper Mario 3DS cannot start.%c%c%s%c%cPress START to exit.%c", 10, 10, msg, 10, 10, 10);
    fflush(stdout);
    pm_con_unlock();
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
    }
}

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

void pm_gpu_set_nocolor(int on) {
    if (on == r_nocol) return;
    flush();
    r_nocol = on;
}

void pm_gpu_set_render(int zcmp, int zupd, int blend, int atest, int decal) {
    if (zcmp == r_zcmp && zupd == r_zupd && blend == r_blend && atest == r_atest && decal == r_decal) return;
    flush();
    r_zcmp = zcmp;
    r_zupd = zupd;
    r_blend = blend;
    r_atest = atest;
    r_decal = decal;
}

void pm_gpu_set_tex1(float sx, float sy, float ox, float oy) {
    if (sx == t1_s[0] && sy == t1_s[1] && ox == t1_s[2] && oy == t1_s[3]) return;
    flush();
    t1_s[0] = sx; t1_s[1] = sy; t1_s[2] = ox; t1_s[3] = oy;
}

void pm_gpu_set_plan(const PmPlan* p) {
    if (combine == 8 && memcmp(&plan, p, sizeof plan) == 0) return;
    flush();
    plan = *p;
    combine = 8;
}

void pm_gpu_set_combine(int mode) {
    if (mode == combine) return;
    flush();
    combine = mode;
}

/* The RDP can brighten past 1 in the second cycle. Texture * color * 2 gives that range. */
void pm_gpu_set_rgb_scale(int scale) {
    if (scale == rgb_scale) return;
    flush();
    rgb_scale = scale;
}

void pm_gpu_set_prim(unsigned rgba) {
    if ((combine == 5 || combine == 6 || combine == 7) && rgba != prim) flush();
    prim = rgba;
}

void pm_gpu_set_mix(int on, unsigned rgba) {
    if (!on) rgba = 0;
    if (on == r_mix && rgba == r_mixcol) return;
    flush();
    r_mix = on;
    r_mixcol = rgba;
}

void pm_gpu_set_env(unsigned rgba) { env_col = rgba; }

void pm_gpu_set_const(unsigned rgba) {
    if (rgba == cconst) return;
    if (combine == 4) flush();
    cconst = rgba;
}

void pm_gpu_flush(void) { flush(); }

void pm_gpu_set_scissor(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 320) x1 = 320;
    if (y1 > 240) y1 = 240;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    /* V05: the batch in flight was drawn under the old scissor on the N64. Draw it before the scissor changes: the last
     * background piece of a window (its bottom right quarter) was cut by the next pass's scissor, a smaller one. */
    if (x0 != sci_x0 || y0 != sci_y0 || x1 != sci_x1 || y1 != sci_y1) flush();
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
static float px(int x) { return off_active ? (float)x : 40.f + (float)x; }
static float py(int y) { return off_active ? (float)y : 240.f - (float)y; }

static float fill_z = 0.5f;
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
    pm_gpu_tri(xa, ya, fill_z, 0, 0, rgba, xb, ya, fill_z, 0, 0, rgba, xb, yb, fill_z, 0, 0, rgba);
    pm_gpu_tri(xa, ya, fill_z, 0, 0, rgba, xb, yb, fill_z, 0, 0, rgba, xa, yb, fill_z, 0, 0, rgba);
    flush();
    rect_2d = 0;
    combine = saved;
}

void pm_gpu_fill_rect_z(int x0, int y0, int x1, int y1, unsigned rgba, float d) {
    flush(); /* the batch in flight keeps its own depth state */
    fill_z = d;
    rect_z = 1;
    pm_gpu_fill_rect(x0, y0, x1, y1, rgba);
    rect_z = 0;
    fill_z = 0.5f;
}

void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; float s = s0; s0 = s1; s1 = s; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; float s = t0; t0 = t1; t1 = s; }
    flush();
    int saved = combine;
    combine = (saved == 5 || saved == 6 || saved == 7 || saved == 8) ? saved : 2;
    rect_2d = 1;
    batch_identity = 1;
    float xa = px(x0);
    float xb = px(x1 + 1);
    float ya = py(y0);
    float yb = py(y1 + 1);
    unsigned c = (saved == 5 || saved == 7) ? env_col : (saved == 8 && plan.vtxname != PM_NM_SHADE) ? plan.vtx : 0xffffffffu;
    pm_gpu_tri(xa, ya, 0.5f, s0, t0, c, xb, ya, 0.5f, s1, t0, c, xb, yb, 0.5f, s1, t1, c);
    pm_gpu_tri(xa, ya, 0.5f, s0, t0, c, xb, yb, 0.5f, s1, t1, c, xa, yb, 0.5f, s0, t1, c);
    flush();
    rect_2d = 0;
    combine = saved;
}
