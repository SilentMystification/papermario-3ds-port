#ifndef PM_GPU_H
#define PM_GPU_H

void pm_gpu_init(void);
void pm_gpu_begin(int clear);
void pm_gpu_end(void);
/* Two lines at the top of the bottom screen. The log scrolls underneath. */
void pm_gpu_hud(const char* line0, const char* line1);
int pm_gpu_drew(void);
void pm_gpu_read_frame(unsigned short* dst);
int pm_gpu_depth_at(int x, int y, unsigned* d24); /* F03 */

/* Row-vector F3DEX matrix: out = v * m. The shader multiplies object-space
 * vertices by this, then the viewport and the screen tilt. */
void pm_gpu_set_mvp(const float m[4][4], float vp_sx, float vp_sy, float vp_tx, float vp_ty);
void pm_gpu_set_mode(int zbuffer, int lighting);
void pm_gpu_set_rgb_scale(int scale); /* 1 or 2, texture stage only */
void pm_gpu_set_combine(int mode); /* 0 shade, 1 primitive, 2 texture * shade */

/* S25: a general RDP combiner as PICA texenv stages (mode 8). The stages are built by pm_comb.c. */
enum { PM_S_TEX0, PM_S_VTX, PM_S_CONST, PM_S_PREV, PM_S_BUF, PM_S_TEX1 };
enum { PM_O_C, PM_O_1MC, PM_O_A, PM_O_1MA };
enum { PM_F_REPLACE, PM_F_MODULATE, PM_F_ADD, PM_F_SUBTRACT, PM_F_INTERP, PM_F_MULADD };
enum { PM_NM_NONE, PM_NM_ZERO, PM_NM_PRIM, PM_NM_ENV, PM_NM_PLOD, PM_NM_SHADE, PM_NM_PK, PM_NM_KEYC, PM_NM_KEYS, PM_NM_HALF }; /* PK: PRIM - CENTER, B6 */
typedef struct {
    unsigned char src[3], op[3], func; /* one channel group of one stage */
    unsigned char scale; /* 0 or 1: x1, 2 or 4: the PICA stage scale (result x scale, then clamp) */
} PmTevCh;
typedef struct {
    PmTevCh g[2];       /* 0 colour, 1 alpha */
    unsigned char cname; /* what the stage constant holds (PM_NM_*) */
    unsigned constant;  /* RGBA 0xRRGGBBAA, from cname */
} PmTevStage;
typedef struct {
    int n;               /* stages in use, 1 to 6 */
    PmTevStage st[6];
    unsigned char bufname, vtxname; /* PM_NM_*: the buffer colour, and what the vertex colour carries */
    unsigned buf, vtx;   /* RGBA, from the names */
    unsigned char uses_tex, uses_sha, uses_tex1; /* a stage reads TEXTURE0, the vertex alpha as shade alpha, TEXTURE1 */
} PmPlan;
void pm_gpu_set_plan(const PmPlan* p);
/* Plan with TEXTURE1: its coordinates are uv * (sx, sy) + (ox, oy). The caller has made the unit 1 texture (pm_tex_set_aux). */
void pm_gpu_set_tex1(float sx, float sy, float ox, float oy);

/* pm_comb.c. Build the stages of one G_SETCOMBINE (two cycles when `two`). t1only: TEXEL1 is the bound texture.
 * Returns 0 and fills `out`, or a reason: 1 more than 6 stages, 2 (unused), 3 COMBINED read after a
 * sub-stage, 4 more constants than the PICA has, 5 no vertex or texture source for a name. */
int pm_comb_build(unsigned w0, unsigned w1, int two, int t1only, PmPlan* out);
void pm_comb_swap_c2(unsigned s[8]);
/* Fill the constants. prim, env RGBA 0xRRGGBBAA, plod 0..255 (PRIM_LOD_FRAC), key and keys RGBA 0xRRGGBB00 (CENTER, SCALE). */
void pm_comb_colours(PmPlan* p, unsigned prim, unsigned env, unsigned plod, unsigned key, unsigned keys);
/* What the stages give, as the PICA computes them. t0, shade: RGBA 0..1. out: RGBA 0..1. */
void pm_comb_eval(const PmPlan* p, const float t0[4], const float t1[4], const float shade[4], float out[4]);
void pm_gpu_set_const(unsigned rgba); /* combine mode 4: flat colour (a = factor on texture alpha) */
void pm_gpu_set_zmark(int on, float zn); /* B3b: case 6 model: depth test against the world, constant depth write */
void pm_gpu_set_nocolor(int on); /* S25: the blender returns the frame buffer colour, so only depth is written */
void pm_gpu_set_render(int zcmp, int zupd, int blend, int atest, int decal);
void pm_gpu_set_prim(unsigned rgba);
/* V06: a last stage mixes the colour to rgb of `rgba` by its alpha (the RDP blender with the blend or fog colour and the fog alpha). */
void pm_gpu_set_mix(int on, unsigned rgba);
void pm_gpu_set_env(unsigned rgba); /* combine mode 5: lerp from ENV (vertex colour) to PRIM by texel */
/* Draw the current batch now. The next vertices belong to a different texture. */
void pm_gpu_flush(void);
/* N64 pixels, y down. Clipped to the 320x240 picture inside the 400-wide target. */
void pm_gpu_set_scissor(int x0, int y0, int x1, int y1);

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2);
/* Object-space triangle. The current MVP uniform transforms it. w is 1. */
void pm_gpu_tri3d(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                  float x1, float y1, float z1, float u1, float v1, unsigned c1,
                  float x2, float y2, float z2, float u2, float v2, unsigned c2);
/* One vertex already in F3DEX clip space. The MVP uniform must be identity. */
void pm_gpu_vert_clip(float x, float y, float z, float w, float u, float v, unsigned c);
/* V05: draws go to an off-screen RGBA image for the colour image at base (width stride pixels) until pm_gpu_off_end. */
int pm_gpu_off_begin(const void* base, int stride, int siz); /* siz: 3 = 32 bit, 2 = 16 bit, 1 = 8 bit */
void pm_gpu_off_end(void);
int pm_gpu_off_count(void); /* B1: off-screen slots started this frame */
/* B2: T2 grab. Copies the full width of the frame being drawn (rows y0..y1-1 are the window: the copy is reused when texrects drew only other rows) into the upright slot (cfb, siz 2).
 * Returns 0 = no slot, 1 = slot reused, 2 = new copy (the command list was split). */
int pm_gpu_grab(const void* cfb, int y0, int y1);
void pm_gpu_set_replace(int on); /* off-screen fill: write the pixel, no blend */
void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba);
void pm_gpu_fill_depth(int x0, int y0, int x1, int y1, float d); /* B3: depth write only, d 0 near .. 1 far */
void pm_gpu_tex_rect_z(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1, float d); /* B3: tests depth d */
void pm_gpu_fill_rect_z(int x0, int y0, int x1, int y1, unsigned rgba, float d); /* X01: tests depth d (0 near .. 1 far) */
void pm_gpu_tex_rect(int x0, int y0, int x1, int y1,
                     float s0, float t0, float s1, float t1);

#endif
