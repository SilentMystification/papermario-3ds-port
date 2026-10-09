#ifndef PM_GPU_H
#define PM_GPU_H

void pm_gpu_init(void);
void pm_gpu_begin(int clear);
void pm_gpu_end(void);
/* Two lines at the top of the bottom screen. The log scrolls underneath. */
void pm_gpu_hud(const char* line0, const char* line1);
int pm_gpu_drew(void);

/* Row-vector F3DEX matrix: out = v * m. The shader multiplies object-space
 * vertices by this, then the viewport and the screen tilt. */
void pm_gpu_set_mvp(const float m[4][4], float vp_sx, float vp_sy, float vp_tx, float vp_ty);
void pm_gpu_set_mode(int zbuffer, int lighting);
void pm_gpu_set_combine(int mode); /* 0 shade, 1 primitive, 2 texture * shade */
void pm_gpu_set_prim(unsigned rgba);
/* N64 pixels, y down. Clipped to the 320x240 picture inside the 400-wide target. */
void pm_gpu_set_scissor(int x0, int y0, int x1, int y1);

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2);
/* Object-space triangle. The current MVP uniform transforms it. */
void pm_gpu_tri3d(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                  float x1, float y1, float z1, float u1, float v1, unsigned c1,
                  float x2, float y2, float z2, float u2, float v2, unsigned c2);
void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba);
void pm_gpu_tex_rect(int x0, int y0, int x1, int y1,
                     float s0, float t0, float s1, float t1);

#endif
