#ifndef PM_GPU_H
#define PM_GPU_H

void pm_gpu_init(void);
void pm_gpu_begin(void);
void pm_gpu_end(void);
int pm_gpu_drew(void);

/* m is row-major, column-vector: out_i = sum_j m[i][j] * in_j. */
void pm_gpu_set_mvp(const float m[4][4]);
void pm_gpu_set_mode(int zbuffer, int lighting);
void pm_gpu_set_combine(int mode); /* 0 shade, 1 primitive, 2 texture * shade */
void pm_gpu_set_prim(unsigned rgba);

void pm_gpu_tri(float x0, float y0, float z0, float u0, float v0, unsigned c0,
                float x1, float y1, float z1, float u1, float v1, unsigned c1,
                float x2, float y2, float z2, float u2, float v2, unsigned c2);
void pm_gpu_fill_rect(int x0, int y0, int x1, int y1, unsigned rgba);
void pm_gpu_tex_rect(int x0, int y0, int x1, int y1,
                     float s0, float t0, float s1, float t1);

#endif
