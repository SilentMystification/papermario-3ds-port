/* S0 BENCH: micro-benchmarks for the O3DS redesign (docs/plans/arch_review.md part 3, B1 to B7).
 * The benches own their render target, buffers, textures and programs. They read no game state. The only game calls are in the
 * item `drawpm` (pm_gpu_begin, pm_gpu_vert_clip, pm_gpu_flush, pm_gpu_end), which resets its per-frame state at the next pm_gpu_begin.
 * Switches: `bench` (all except r2 and svcflush) or `bench=<list>`, `benchexit`. Read docs/plans/bench_notes.md.
 * Every result line starts with `bench:` and holds min, avg and max over repeated runs and the unit. */
#include "pm_build.h"
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "pm_shader_shbin.h"
#include "pm_shader3_shbin.h"
#include "pm_port.h"
#include "pm_gpu.h"
#include "pm_bench.h"

#define RUNS 5
#define US_OF(t) ((double)(t) * 1e6 / (double)SYSCLOCK_ARM11)

/* ---- statistics and output ---- */
typedef struct { double mn, mx, sum; int n; } St;
static void st_init(St* s) { s->mn = 1e300; s->mx = -1e300; s->sum = 0; s->n = 0; }
static void st_add(St* s, double v) { if (v < s->mn) s->mn = v; if (v > s->mx) s->mx = v; s->sum += v; s->n++; }
static double st_avg(const St* s) { return s->n ? s->sum / s->n : 0; }
/* append "<name> min a avg b max c" to a line buffer */
static void seg(char* line, unsigned cap, const char* name, const St* s) {
    size_t n = strlen(line);
    snprintf(line + n, cap - n, " %s min %.2f avg %.2f max %.2f", name, s->n ? s->mn : 0.0, st_avg(s), s->n ? s->mx : 0.0);
}
static void emit(const char* line) { pm_log_w("bench: %s", line); pm_log_poll(); }
static int alive(void) { pm_log_poll(); return aptMainLoop(); }
static u64 tk(void) { return svcGetSystemTick(); }
static void spin_ms(unsigned ms) { u64 e = tk() + (u64)ms * (SYSCLOCK_ARM11 / 1000); while (tk() < e) {} }

/* ---- selection ---- */
enum { S_DRAW = 1, S_DRAWPM = 2, S_R2 = 4, S_FLUSH = 8, S_SVC = 16, S_TEX = 32, S_REPLAY = 64, S_FBWAIT = 128 };
static unsigned parse_sel(void) {
    char v[64];
    if (pm_debug_value("bench", v, sizeof v)) {
        static const struct { const char* n; unsigned b; } tab[] = { { "draw", S_DRAW }, { "drawpm", S_DRAWPM }, { "r2", S_R2 }, { "flush", S_FLUSH },
            { "svcflush", S_SVC }, { "tex", S_TEX }, { "replay", S_REPLAY }, { "fbwait", S_FBWAIT } };
        unsigned sel = 0;
        for (char* p = v; *p;) {
            char* e = strchr(p, ',');
            size_t l = e ? (size_t)(e - p) : strlen(p);
            int hit = 0;
            for (unsigned i = 0; i < sizeof tab / sizeof tab[0]; i++) if (strlen(tab[i].n) == l && !strncmp(tab[i].n, p, l)) { sel |= tab[i].b; hit = 1; }
            if (!hit) pm_log_w("bench: unknown name %.*s (draw drawpm r2 flush svcflush tex replay fbwait)", (int)l, p);
            p += l + (e ? 1 : 0);
        }
        return sel;
    }
    if (pm_debug_has("bench")) return S_DRAW | S_DRAWPM | S_FLUSH | S_TEX | S_REPLAY | S_FBWAIT; /* r2 and svcflush only by name */
    return 0;
}

/* ---- GPU setup: own target, own programs ---- */
typedef struct { s16 x, y, z, f, s, t; u8 c[4]; } V3;          /* 16 bytes, shader pm3: short4, short2, ubyte4 */
typedef struct { float p[4], t[2], c[4]; } VA;                  /* 40 bytes, shader pm: float4, float2, float4 */
#define MAXK 8
static C3D_RenderTarget* rt;
static DVLB_s *dvA, *dvB;
static shaderProgram_s pA, pB;
static int uA_proj, uB_proj, uB_uvso, uB_cmul, uB_cadd, uB_zmix;
static C3D_AttrInfo atA, atB;
static C3D_Mtx proj;
static V3* vb3;
static VA* vba;
static C3D_Tex btex[2];
static int gpu_ready;

static void tri_pos(int j, int* x, int* y) { *x = 2 + 5 * (j % 10); *y = 2 + 5 * ((j / 10) % 10); }
static int gpu_setup(void) {
    if (gpu_ready) return 1;
    rt = C3D_RenderTargetCreate(64, 64, GPU_RB_RGBA8, -1);
    if (!rt) { pm_log_w("bench: gpu setup failed (render target)"); return 0; }
    dvA = DVLB_ParseFile((u32*)pm_shader_shbin, pm_shader_shbin_size);
    dvB = DVLB_ParseFile((u32*)pm_shader3_shbin, pm_shader3_shbin_size);
    shaderProgramInit(&pA); shaderProgramSetVsh(&pA, &dvA->DVLE[0]);
    shaderProgramInit(&pB); shaderProgramSetVsh(&pB, &dvB->DVLE[0]);
    uA_proj = shaderInstanceGetUniformLocation(pA.vertexShader, "projection");
    uB_proj = shaderInstanceGetUniformLocation(pB.vertexShader, "projection");
    uB_uvso = shaderInstanceGetUniformLocation(pB.vertexShader, "uvso");
    uB_cmul = shaderInstanceGetUniformLocation(pB.vertexShader, "cmul");
    uB_cadd = shaderInstanceGetUniformLocation(pB.vertexShader, "cadd");
    uB_zmix = shaderInstanceGetUniformLocation(pB.vertexShader, "zmix");
    AttrInfo_Init(&atA); AttrInfo_AddLoader(&atA, 0, GPU_FLOAT, 4); AttrInfo_AddLoader(&atA, 1, GPU_FLOAT, 2); AttrInfo_AddLoader(&atA, 2, GPU_FLOAT, 4);
    AttrInfo_Init(&atB); AttrInfo_AddLoader(&atB, 0, GPU_SHORT, 4); AttrInfo_AddLoader(&atB, 1, GPU_SHORT, 2); AttrInfo_AddLoader(&atB, 2, GPU_UNSIGNED_BYTE, 4);
    Mtx_OrthoTilt(&proj, 0.f, 64.f, 0.f, 64.f, 0.f, 1.f, true);
    vb3 = (V3*)linearAlloc(sizeof(V3) * 3 * MAXK);
    vba = (VA*)linearAlloc(sizeof(VA) * 3 * MAXK);
    if (!vb3 || !vba || uB_proj < 0 || uA_proj < 0) { pm_log_w("bench: gpu setup failed (buffers or uniforms %d %d)", uA_proj, uB_proj); return 0; }
    for (int j = 0; j < MAXK; j++) {
        int x, y; tri_pos(j, &x, &y);
        for (int v = 0; v < 3; v++) {
            V3* a = &vb3[j * 3 + v]; VA* b = &vba[j * 3 + v];
            int dx = v == 1 ? 3 : 0, dy = v == 2 ? 3 : 0;
            a->x = (s16)(x + dx); a->y = (s16)(y + dy); a->z = 0; a->f = 1; a->s = 0; a->t = 0; a->c[0] = 255; a->c[1] = 128; a->c[2] = 64; a->c[3] = 255;
            b->p[0] = (float)(x + dx); b->p[1] = (float)(y + dy); b->p[2] = 0.f; b->p[3] = 1.f; b->t[0] = b->t[1] = 0.f;
            b->c[0] = 1.f; b->c[1] = .5f; b->c[2] = .25f; b->c[3] = 1.f;
        }
    }
    GSPGPU_FlushDataCache(vb3, sizeof(V3) * 3 * MAXK);
    GSPGPU_FlushDataCache(vba, sizeof(VA) * 3 * MAXK);
    for (int i = 0; i < 2; i++) {
        if (!C3D_TexInit(&btex[i], 8, 8, GPU_RGBA8)) { pm_log_w("bench: gpu setup failed (texture)"); return 0; }
        memset(btex[i].data, 0x40 + i * 0x40, 8 * 8 * 4);
        C3D_TexFlush(&btex[i]);
    }
    gpu_ready = 1;
    return 1;
}
static void gpu_teardown(void) {
    if (!gpu_ready) return;
    C3D_FrameBegin(0); C3D_FrameDrawOn(rt); C3D_FrameEnd(0); /* the last frame is done when this call returns the next time */
    C3D_FrameBegin(0); C3D_FrameDrawOn(rt); C3D_FrameEnd(0);
    for (int i = 0; i < 2; i++) C3D_TexDelete(&btex[i]);
    linearFree(vb3); linearFree(vba);
    C3D_RenderTargetDelete(rt);
    shaderProgramFree(&pA); shaderProgramFree(&pB);
    DVLB_Free(dvA); DVLB_Free(dvB);
    gpu_ready = 0;
}
static void frame_begin(void) {
    C3D_FrameBegin(0);
    C3D_FrameDrawOn(rt);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, 8, 8);
}
/* GPU time of the frame before: the next FrameBegin(0) returns when it is done */
static double gpu_us_of_last(void) {
    C3D_FrameBegin(0);
    double g = (double)C3D_GetDrawingTime() * 1000.0;
    C3D_FrameDrawOn(rt);
    C3D_FrameEnd(0);
    return g;
}
static void bind_b(const void* vtx, int stride_is_b) {
    C3D_BindProgram(&pB);
    C3D_SetAttrInfo(&atB);
    C3D_BufInfo* bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, vtx, stride_is_b ? (int)sizeof(V3) : (int)sizeof(VA), 3, 0x210);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uB_proj, &proj);
}
static void bind_a(void) {
    C3D_BindProgram(&pA);
    C3D_SetAttrInfo(&atA);
    C3D_BufInfo* bi = C3D_GetBufInfo();
    BufInfo_Init(bi);
    BufInfo_Add(bi, vba, (int)sizeof(VA), 3, 0x210);
}
static void stages(int n) {
    for (int i = 0; i < n; i++) {
        C3D_TexEnv* e = C3D_GetTexEnv(i);
        C3D_TexEnvInit(e);
        if (i == 0) { C3D_TexEnvSrc(e, C3D_Both, GPU_PRIMARY_COLOR, 0, 0); C3D_TexEnvFunc(e, C3D_Both, GPU_REPLACE); }
        else { C3D_TexEnvSrc(e, C3D_Both, GPU_PREVIOUS, GPU_CONSTANT, 0); C3D_TexEnvFunc(e, C3D_Both, GPU_MODULATE); }
    }
}

/* ---- B1: the draw ladder ---- */
enum { D0, D1, D2, D3, D4, D5, R0, R1, R2, ND };
static const char* const dname[ND] = { "d0", "d1", "d2", "d3", "d4", "d5", "r0", "r1", "r2" };
static const char* const dinfo[ND] = { "plain", "1_texenv", "6_texenv", "texbind_x2", "5_uniforms", "program_x2", "raw_draw", "raw_state_draw", "gpu_call" };
static u32 pk_draw[128], pk_state[512];
static unsigned pk_nd, pk_ns;
static int pk_split;
static u32* sub_buf; /* R2: the sub command buffer in linear memory */
static unsigned sub_words;

static void capture(int k) {
    u32 *a, sz, o0, o1;
    GPUCMD_GetBuffer(&a, &sz, &o0);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k);
    GPUCMD_GetBuffer(&a, &sz, &o1);
    pk_nd = o1 - o0 > 128 ? 128 : o1 - o0;
    memcpy(pk_draw, a + o0, pk_nd * 4);
    stages(6);
    GPUCMD_GetBuffer(&a, &sz, &o0);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k);
    GPUCMD_GetBuffer(&a, &sz, &o1);
    unsigned tot = o1 - o0;
    if (tot > 512) tot = 512;
    if (tot > pk_nd && !memcmp(a + o0 + tot - pk_nd, pk_draw, pk_nd * 4)) { pk_ns = tot - pk_nd; pk_split = 1; }
    else { pk_ns = tot; pk_split = 0; }
    memcpy(pk_state, a + o0, pk_ns * 4);
}

static void do_item(int item, int n, int k) {
    u32 pa = 0;
    if (item == R2) pa = osConvertVirtToPhys(sub_buf);
    if (item == R2) { GPUCMD_AddWrite(GPUREG_CMDBUF_SIZE0, sub_words >> 1); GPUCMD_AddWrite(GPUREG_CMDBUF_ADDR0, pa >> 3); }
    for (int i = 0; i < n; i++) {
        switch (item) {
        case D0: C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k); break;
        case D1: stages(1); C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k); break;
        case D2: stages(6); C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k); break;
        case D3: C3D_TexBind(0, &btex[i & 1]); C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k); break;
        case D4:
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uB_uvso, 1.f, 1.f, (float)(i & 7), 0.f);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uB_cmul, 1.f, 1.f, 1.f, 1.f);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uB_cadd, 0.f, 0.f, 0.f, 0.f);
            C3D_FVUnifSet(GPU_VERTEX_SHADER, uB_zmix, 1.f, 0.f, 0.f, 0.f);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uB_proj, &proj);
            C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k);
            break;
        case D5:
            if (i & 1) bind_a(); else bind_b(vb3, 1);
            C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k);
            break;
        case R0: GPUCMD_AddRawCommands(pk_draw, pk_nd); break;
        case R1:
            if (pk_split) { GPUCMD_AddRawCommands(pk_state, pk_ns); GPUCMD_AddRawCommands(pk_draw, pk_nd); }
            else GPUCMD_AddRawCommands(pk_state, pk_ns);
            break;
        case R2: GPUCMD_AddWrite(GPUREG_CMDBUF_JUMP0, 1); break;
        }
    }
}

static void bench_draw(unsigned sel) {
    static const int ns[] = { 1, 10, 100, 500 }, ks[] = { 1, MAXK };
    if (!gpu_setup()) return;
    sub_buf = NULL;
    for (int ki = 0; ki < 2; ki++) {
        int k = ks[ki];
        for (int item = 0; item < ND; item++) {
            if (item == R2 && !(sel & S_R2)) continue;
            for (int ni = 0; ni < 4; ni++) {
                int n = ns[ni];
                if (!alive()) return;
                St cpu, gpu; st_init(&cpu); st_init(&gpu);
                for (int r = 0; r < RUNS; r++) {
                    frame_begin();
                    bind_b(vb3, 1);
                    C3D_DrawArrays(GPU_TRIANGLES, 0, 3 * k); /* warm-up: all dirty state goes out here */
                    if (item >= R0) {
                        capture(k);
                        if (item == R2) {
                            if (!sub_buf) sub_buf = (u32*)linearAlloc(512);
                            if (!sub_buf) { pm_log_w("bench: r2 buffer alloc failed"); C3D_FrameEnd(0); return; }
                            sub_words = pk_nd & ~1u;
                            memcpy(sub_buf, pk_draw, sub_words * 4);
                            GSPGPU_FlushDataCache(sub_buf, sub_words * 4);
                        }
                        bind_b(vb3, 1);
                    }
                    u64 t0 = tk();
                    do_item(item, n, k);
                    u64 t1 = tk();
                    C3D_FrameEnd(0);
                    st_add(&cpu, US_OF(t1 - t0) / n);
                    st_add(&gpu, gpu_us_of_last());
                }
                char line[200];
                snprintf(line, sizeof line, "draw %s(%s) n %d k %d", dname[item], dinfo[item], n, k);
                seg(line, sizeof line, "cpu_us_per_draw", &cpu);
                seg(line, sizeof line, "gpu_us_per_frame", &gpu);
                if (item >= R0 && item <= R1) { size_t l = strlen(line); snprintf(line + l, sizeof line - l, " words draw %u state %u", pk_nd, pk_split ? pk_ns : 0u); }
                snprintf(line + strlen(line), sizeof line - strlen(line), " runs %d", RUNS);
                emit(line);
            }
        }
    }
    if (sub_buf) { linearFree(sub_buf); sub_buf = NULL; }
}

/* d6: the full pm_gpu path. One triangle and a plan change per draw (pm_gpu_flush forces the draw). Touches the game GPU module. */
static void bench_drawpm(void) {
    static const int ns[] = { 1, 10, 100, 500 };
    for (int ni = 0; ni < 4; ni++) {
        int n = ns[ni];
        if (!alive()) return;
        St cpu; st_init(&cpu);
        for (int r = 0; r < RUNS; r++) {
            pm_gpu_begin(0);
            pm_gpu_set_scissor(0, 0, 8, 8);
            u64 t0 = tk();
            for (int i = 0; i < n; i++) {
                pm_gpu_set_combine(i & 1);
                pm_gpu_vert_clip(-.05f, -.05f, .5f, 1.f, 0.f, 0.f, 0xff8040ffu);
                pm_gpu_vert_clip(.05f, -.05f, .5f, 1.f, 0.f, 0.f, 0xff8040ffu);
                pm_gpu_vert_clip(-.05f, .05f, .5f, 1.f, 0.f, 0.f, 0xff8040ffu);
                pm_gpu_flush();
            }
            u64 t1 = tk();
            pm_gpu_end();
            st_add(&cpu, US_OF(t1 - t0) / n);
        }
        char line[200];
        snprintf(line, sizeof line, "draw d6(pm_gpu_path) n %d k 1", n);
        seg(line, sizeof line, "cpu_us_per_draw", &cpu);
        snprintf(line + strlen(line), sizeof line - strlen(line), " runs %d", RUNS);
        emit(line);
    }
}

/* ---- B2: flush cost ---- */
static volatile int lt_stop;
static u8* lt_buf;
static void lt_fn(void* arg) { /* audio proxy: ALU and memory load like the mixer */
    (void)arg;
    u32 x = 1;
    while (!lt_stop) {
        u32* p = (u32*)lt_buf;
        for (int i = 0; i < 16384; i++) { p[i] = p[i] * x + (u32)i; x += 3; }
    }
}
static Thread lt;
static int load_start(void) {
    lt_buf = (u8*)malloc(65536);
    if (!lt_buf) return 0;
    memset(lt_buf, 1, 65536);
    lt_stop = 0;
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    lt = threadCreate(lt_fn, NULL, 16 * 1024, prio, 1, false);
    return lt != NULL;
}
static void load_stop(void) {
    if (!lt) return;
    lt_stop = 1;
    threadJoin(lt, U64_MAX);
    threadFree(lt);
    lt = NULL;
    free(lt_buf);
    lt_buf = NULL;
}

#define FL_NS 7
static const unsigned fl_size[FL_NS] = { 64, 1024, 4096, 16384, 65536, 262144, 1048576 };
static void bench_flush(unsigned sel) {
    unsigned cap = 1048576;
    u8* buf = (u8*)linearAlloc(cap);
    if (!buf) { cap = 262144; buf = (u8*)linearAlloc(cap); }
    if (!buf) { pm_log_w("bench: flush buffer alloc failed"); return; }
    memset(buf, 0, cap);
    static double fit[2][2][FL_NS]; /* [audio][dirty][size] avg us */
    for (int audio = 0; audio < 2; audio++) {
        if (audio && !load_start()) { pm_log_w("bench: flush audio 1 not available (load thread failed)"); break; }
        if (audio) svcSleepThread(50000000ll);
        for (int dirty = 0; dirty < 2; dirty++) {
            for (int si = 0; si < FL_NS; si++) {
                unsigned sz = fl_size[si];
                if (sz > cap) continue;
                if (!alive()) { load_stop(); linearFree(buf); return; }
                int calls = sz <= 65536 ? 50 : 10;
                St st; st_init(&st);
                for (int r = 0; r < RUNS; r++) {
                    u64 acc = 0;
                    for (int c = 0; c < calls; c++) {
                        if (dirty) memset(buf, r + c, sz);
                        u64 t0 = tk();
                        GSPGPU_FlushDataCache(buf, sz);
                        acc += tk() - t0;
                    }
                    st_add(&st, US_OF(acc) / calls);
                }
                fit[audio][dirty][si] = st_avg(&st);
                char line[200];
                snprintf(line, sizeof line, "flush gsp size %u dirty %d audio_proxy %d", sz, dirty, audio);
                seg(line, sizeof line, "us_per_call", &st);
                snprintf(line + strlen(line), sizeof line - strlen(line), " calls %d runs %d", calls, RUNS);
                emit(line);
            }
            if (cap >= 1048576) {
                double per_kb = (fit[audio][dirty][FL_NS - 1] - fit[audio][dirty][0]) / (1048576.0 / 1024.0 - 64.0 / 1024.0);
                char line[200];
                snprintf(line, sizeof line, "flush fit gsp dirty %d audio_proxy %d fixed_us %.2f per_kb_us %.3f", dirty, audio, fit[audio][dirty][0] - per_kb / 16.0, per_kb);
                emit(line);
            }
        }
        /* GX queue: the cost of the enqueue only. A pause lets the queue drain. */
        for (int si = 0; si < FL_NS; si += 2) {
            unsigned sz = fl_size[si];
            if (sz > cap) continue;
            St st; st_init(&st);
            for (int r = 0; r < RUNS; r++) {
                u64 acc = 0;
                for (int c = 0; c < 8; c++) {
                    memset(buf, c, sz);
                    u64 t0 = tk();
                    GX_FlushCacheRegions((u32*)buf, sz, NULL, 0, NULL, 0);
                    acc += tk() - t0;
                    svcSleepThread(3000000ll);
                }
                st_add(&st, US_OF(acc) / 8);
            }
            char line[200];
            snprintf(line, sizeof line, "flush gx_enqueue size %u dirty 1 audio_proxy %d", sz, audio);
            seg(line, sizeof line, "us_per_call", &st);
            snprintf(line + strlen(line), sizeof line - strlen(line), " calls 8 runs %d", RUNS);
            emit(line);
        }
        if (audio) load_stop();
    }
    if (sel & S_SVC) { /* an SVC that the process may not call ends the process: log first */
        pm_log_w("bench: svcflush start (the process ends here if the SVC is not allowed)");
        svcSleepThread(300000000ll);
        for (int si = 0; si < FL_NS; si += 2) {
            unsigned sz = fl_size[si];
            if (sz > cap) continue;
            St st; st_init(&st);
            for (int r = 0; r < RUNS; r++) {
                u64 acc = 0;
                for (int c = 0; c < 20; c++) {
                    memset(buf, c, sz);
                    u64 t0 = tk();
                    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)buf, sz);
                    acc += tk() - t0;
                }
                st_add(&st, US_OF(acc) / 20);
            }
            char line[200];
            snprintf(line, sizeof line, "flush svc size %u dirty 1 audio_proxy 0", sz);
            seg(line, sizeof line, "us_per_call", &st);
            snprintf(line + strlen(line), sizeof line - strlen(line), " calls 20 runs %d", RUNS);
            emit(line);
        }
    }
    linearFree(buf);
}

/* ---- B3: texture stages and write speed ---- */
static void bench_wr(void) {
    const unsigned sz = 65536;
    u8* lin = (u8*)linearAlloc(sz);
    u8* vram = (u8*)vramAlloc(sz);
    u8* src = (u8*)malloc(sz);
    if (!lin || !src) { pm_log_w("bench: wr alloc failed"); goto done; }
    for (unsigned i = 0; i < sz; i++) src[i] = (u8)(i * 7);
    for (int tgt = 0; tgt < 2; tgt++) {
        u8* d = tgt ? vram : lin;
        if (!d) { pm_log_w("bench: wr vram alloc failed"); continue; }
        for (int kind = 0; kind < 3; kind++) {
            for (int fl = 0; fl < (tgt ? 1 : 2); fl++) {
                St st; st_init(&st);
                for (int r = 0; r < RUNS; r++) {
                    u64 t0 = tk();
                    if (kind == 0) { volatile u32* p = (volatile u32*)d; for (unsigned i = 0; i < sz / 4; i++) p[i] = i ^ (u32)r; }
                    else if (kind == 1) { volatile u8* p = d; for (unsigned i = 0; i < sz; i++) p[i] = (u8)(i ^ (unsigned)r); }
                    else memcpy(d, src, sz);
                    if (fl) GSPGPU_FlushDataCache(d, sz);
                    st_add(&st, (double)sz / US_OF(tk() - t0));
                }
                char line[200];
                snprintf(line, sizeof line, "wr %s %s flush %d size %u", tgt ? "vram_uncached" : "linear_cached", kind == 0 ? "word" : kind == 1 ? "byte" : "memcpy", fl, sz);
                seg(line, sizeof line, "bytes_per_us", &st);
                snprintf(line + strlen(line), sizeof line - strlen(line), " runs %d", RUNS);
                emit(line);
            }
        }
    }
done:
    if (lin) linearFree(lin);
    if (vram) vramFree(vram);
    free(src);
}

static void bench_tex(void) {
    enum { BATCH = 50, PASSES = 4 };
    static C3D_Tex tex[BATCH];
    static const u8 zt[64] = { 0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23, 8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31,
        32, 33, 36, 37, 48, 49, 52, 53, 34, 35, 38, 39, 50, 51, 54, 55, 40, 41, 44, 45, 56, 57, 60, 61, 42, 43, 46, 47, 58, 59, 62, 63 };
    u8* src = (u8*)malloc(64 * 64 * 2);
    u16 tlut[256];
    if (!src) return;
    for (unsigned i = 0; i < 64 * 64 * 2; i++) src[i] = (u8)(i * 13 + (i >> 5));
    for (int i = 0; i < 256; i++) tlut[i] = (u16)(i * 257 | 1);
    for (int fmt = 0; fmt < 2; fmt++) {
        int w = fmt ? 64 : 32;
        St si, sd, sf, sx; st_init(&si); st_init(&sd); st_init(&sf); st_init(&sx);
        for (int p = 0; p < PASSES; p++) {
            if (!alive()) { free(src); return; }
            int made = 0;
            u64 t0 = tk();
            for (int i = 0; i < BATCH; i++) { if (!C3D_TexInit(&tex[i], (u16)w, (u16)w, GPU_RGBA5551)) break; made++; }
            u64 t1 = tk();
            for (int i = 0; i < made; i++) { /* decode proxy: RGBA5551 copy or CI8 with a 256 entry TLUT, written in PICA 8x8 tile order */
                u16* d = (u16*)tex[i].data;
                const u16* s16 = (const u16*)src;
                for (int ty = 0; ty < w / 8; ty++) for (int tx = 0; tx < w / 8; tx++) {
                    u16* td = d + (ty * (w / 8) + tx) * 64;
                    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
                        int o = (ty * 8 + y) * w + tx * 8 + x;
                        td[zt[y * 8 + x]] = fmt ? tlut[src[o]] : s16[o];
                    }
                }
            }
            u64 t2 = tk();
            for (int i = 0; i < made; i++) C3D_TexFlush(&tex[i]);
            u64 t3 = tk();
            for (int i = 0; i < made; i++) C3D_TexDelete(&tex[i]);
            u64 t4 = tk();
            if (made) { st_add(&si, US_OF(t1 - t0) / made); st_add(&sd, US_OF(t2 - t1) / made); st_add(&sf, US_OF(t3 - t2) / made); st_add(&sx, US_OF(t4 - t3) / made); }
        }
        char line[300];
        snprintf(line, sizeof line, "tex %dx%d %s us_per_texture batch %d", w, w, fmt ? "ci8_to_rgba5551(decode_is_proxy)" : "rgba5551(decode_is_proxy)", BATCH);
        seg(line, sizeof line, "init", &si); seg(line, sizeof line, "decode", &sd); seg(line, sizeof line, "flush", &sf); seg(line, sizeof line, "free", &sx);
        snprintf(line + strlen(line), sizeof line - strlen(line), " runs %d", PASSES);
        emit(line);
    }
    free(src);
    bench_wr();
}

/* ---- B4: replay of a cached batch ---- */
static void bench_replay(void) {
    static const unsigned tris[] = { 100, 1000, 5000 };
    if (!gpu_setup()) return;
    for (int ti = 0; ti < 3; ti++) {
        unsigned T = tris[ti], nv = T * 3, vbytes = (nv * (unsigned)sizeof(V3) + 15u) & ~15u, ibytes = nv * 2;
        u8* tmp = (u8*)malloc(vbytes + ibytes);
        if (!tmp) return;
        V3* v = (V3*)tmp; u16* ix = (u16*)(tmp + vbytes);
        for (unsigned j = 0; j < T; j++) {
            int x, y; tri_pos((int)(j % 100), &x, &y);
            for (int q = 0; q < 3; q++) {
                V3* a = &v[j * 3 + q];
                a->x = (s16)(x + (q == 1 ? 3 : 0)); a->y = (s16)(y + (q == 2 ? 3 : 0)); a->z = 0; a->f = 1; a->s = 0; a->t = 0;
                a->c[0] = 200; a->c[1] = 100; a->c[2] = 50; a->c[3] = 255;
                ix[j * 3 + q] = (u16)(j * 3 + q);
            }
        }
        for (int mem = 0; mem < 2; mem++) { /* 0 linear, 1 VRAM (written by the CPU) */
            u8* blk = mem ? (u8*)vramAlloc(vbytes + ibytes) : (u8*)linearAlloc(vbytes + ibytes);
            if (!blk) { pm_log_w("bench: replay %s alloc failed for %u triangles", mem ? "vram" : "linear", T); continue; }
            memcpy(blk, tmp, vbytes + ibytes);
            if (!mem) GSPGPU_FlushDataCache(blk, vbytes + ibytes);
            St cpu, gpu, wall; st_init(&cpu); st_init(&gpu); st_init(&wall);
            for (int r = 0; r < RUNS; r++) {
                if (!alive()) { mem ? vramFree(blk) : linearFree(blk); free(tmp); return; }
                u64 w0 = tk();
                frame_begin();
                C3D_BindProgram(&pB);
                C3D_SetAttrInfo(&atB);
                C3D_BufInfo* bi = C3D_GetBufInfo();
                BufInfo_Init(bi);
                BufInfo_Add(bi, blk, (int)sizeof(V3), 3, 0x210);
                u64 t0 = tk();
                for (int d = 0; d < 100; d++) {
                    C3D_Mtx m = proj;
                    m.r[0].x += 0.000001f * (float)(d & 3); /* a new matrix for every draw */
                    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uB_proj, &m);
                    C3D_DrawElements(GPU_TRIANGLES, (int)nv, C3D_UNSIGNED_SHORT, blk + vbytes);
                }
                u64 t1 = tk();
                C3D_FrameEnd(0);
                st_add(&cpu, US_OF(t1 - t0) / 100.0);
                st_add(&gpu, gpu_us_of_last() / 1000.0);
                st_add(&wall, US_OF(tk() - w0) / 1000.0);
            }
            char line[300];
            snprintf(line, sizeof line, "replay %s draws 100 tris_per_draw %u tris_per_frame %u", mem ? "vram" : "linear", T, T * 100);
            seg(line, sizeof line, "cpu_us_per_draw", &cpu);
            seg(line, sizeof line, "gpu_ms", &gpu);
            seg(line, sizeof line, "frame_wall_ms", &wall);
            snprintf(line + strlen(line), sizeof line - strlen(line), " runs %d", RUNS);
            emit(line);
            if (mem) vramFree(blk); else linearFree(blk);
        }
        free(tmp);
    }
}

/* ---- B7: the frame wait A/B ---- */
static void bench_fbwait(void) {
    static const unsigned loads[] = { 0, 12, 20, 30 };
    if (!gpu_setup()) return;
    for (unsigned li = 0; li < 4; li++) {
        for (int mode = 0; mode < 2; mode++) { /* 0 C3D_FRAME_SYNCDRAW (vblank wait), 1 flags 0 (waits for the GPU of the last frame only) */
            St fps, ms; st_init(&fps); st_init(&ms);
            for (int r = 0; r < 3; r++) {
                if (!alive()) return;
                unsigned frames = 0;
                u64 t0 = tk();
                while (tk() - t0 < SYSCLOCK_ARM11) {
                    C3D_FrameBegin(mode == 0 ? C3D_FRAME_SYNCDRAW : 0);
                    C3D_FrameDrawOn(rt);
                    C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, 8, 8);
                    C3D_BindProgram(&pB); C3D_SetAttrInfo(&atB);
                    C3D_BufInfo* bi = C3D_GetBufInfo(); BufInfo_Init(bi); BufInfo_Add(bi, vb3, (int)sizeof(V3), 3, 0x210);
                    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uB_proj, &proj);
                    C3D_DrawArrays(GPU_TRIANGLES, 0, 3);
                    spin_ms(loads[li]); /* the game step: CPU work after FrameBegin */
                    C3D_FrameEnd(0);
                    frames++;
                }
                double s = US_OF(tk() - t0) / 1e6;
                st_add(&fps, frames / s);
                st_add(&ms, s * 1000.0 / frames);
            }
            char line[200];
            snprintf(line, sizeof line, "fbwait %s cpu_work_ms %u", mode ? "nosync_flags0" : "syncdraw", loads[li]);
            seg(line, sizeof line, "frames_per_s", &fps);
            seg(line, sizeof line, "ms_per_frame", &ms);
            snprintf(line + strlen(line), sizeof line - strlen(line), " runs 3");
            emit(line);
        }
    }
}

int pm_bench_run(void) {
    static int done;
    if (done) return 0;
    done = 1;
    unsigned sel = parse_sel();
    if (!sel) return 0;
    u64 t0 = tk();
    pm_log_w("bench: begin sel 0x%x console %s build %s (numbers from an emulator are host numbers)", sel, pm_new3ds ? "N3DS" : "O3DS", PM_BUILD_STR);
    if (sel & S_DRAW) bench_draw(sel);
    if (alive() && (sel & S_DRAWPM)) bench_drawpm();
    if (alive() && (sel & (S_FLUSH | S_SVC))) bench_flush(sel);
    if (alive() && (sel & S_TEX)) bench_tex();
    if (alive() && (sel & S_REPLAY)) bench_replay();
    if (alive() && (sel & S_FBWAIT)) bench_fbwait();
    gpu_teardown();
    pm_log_w("bench: end total_ms %.0f", US_OF(tk() - t0) / 1000.0);
    pm_log_poll();
    if (pm_debug_has("benchexit")) {
        pm_log_w("bench: benchexit, leaving the frame loop");
        svcSleepThread(300000000ll); /* the log writer runs on another core: let it write the last lines */
        return 1;
    }
    return 0;
}
