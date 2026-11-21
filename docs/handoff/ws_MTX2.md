# ws_MTX2: the target map's .nds header, .seam and .dlc reads run on the prefetch thread (Haiku, oracle NATIVE22, plan_maptrans.md 4, after the user's yes)
Objective: the romfs file waits of pm_dlc_preload (xfer2 seam 29 to 49 ms, dlcp 58 to 67 ms, FACT plan_maptrans.md A) leave the load step. The reads start at the
first covered step (draw off, 5 idle steps before the load). Same bytes, other thread. Counter: none of C1 to C12. Metric: xfer2 seam, dlcp, xfer4 pf.
Heap: at most 16 + 13536 + 61642 B (FACT max sizes in build3ds/romfs/shape, 489 maps). Start: main after the MTX0 merge. Edit tool, new lines marked MTX2. Edits in directory order:
1. 3ds/include/pm_port.h:399 (PmXf): add `unsigned pff_n;`. 3ds/src/pm_xfer.c: add ` pf %u` (pm_xf.pff_n) at the end of the MTX0 xfer4 format.
2. 3ds/src/nd_shape.c: before seam_load (:138) declare `FILE* pm_pf_fopen(const char* path, const char* mode); void pm_prefetch_files_release(void);`.
   Before (:146, :152, :219, :225): `    f = fopen(path, "rb");`. After: `    f = pm_pf_fopen(path, "rb"); /* MTX2 */`.
   :264 becomes `if (!seam_get()) { pm_prefetch_files_release(); return; }`. After `(void)dlcp_open();` (:266) add `pm_prefetch_files_release();`.
3. 3ds/src/pm_main.c:490: before `pm_xfer_hidden();` add `pm_map_cover_post(); /* MTX2 */`, declared above pm_step_run. 4. 3ds/src/pm_map.c after map_prefetch (:335):
```c
void pm_map_cover_post(void) { /* MTX2: the first covered step of a map change posts the file reads of the target shape (logic thread) */
    static int posted; s32 a = gGameStatusPtr->areaID, m = gGameStatusPtr->mapID; char sn[24];
    if (get_game_mode() != GAME_MODE_CHANGE_MAP || !(gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) { posted = 0; return; }
    if (posted || a < 0 || a >= (s32)ARRAY_COUNT(gAreas) || m < 0 || m >= gAreas[a].mapCount) return;
    posted = 1; snprintf(sn, sizeof sn, "%s_shape", gAreas[a].maps[m].id); { extern void pm_prefetch_files(const char* shape); pm_prefetch_files(sn); } }
```
5. 3ds/src/pm_prefetch.c: put this block before pf_main (:50). In pf_start (:66) add `LightEvent_Init(&pff_done, RESET_STICKY);`. At :53 after `pf_maps();` add `pff_run();`.
```c
#define PFF_N 3 /* MTX2: the .nds header (16 B), the .seam, the .dlc of one shape */
static char pff_path[PFF_N][48]; static unsigned char* pff_buf[PFF_N]; static unsigned pff_len[PFF_N]; static volatile int pff_busy; static LightEvent pff_done;
static void pff_run(void) { if (!__atomic_load_n(&pff_busy, __ATOMIC_ACQUIRE)) return; /* worker */
    for (int i = 0; i < PFF_N; i++) { FILE* f = fopen(pff_path[i], "rb"); long n = -1; if (f && fseek(f, 0, SEEK_END) == 0) n = ftell(f); if (i == 0 && n > 16) n = 16;
        if (f && n > 0 && fseek(f, 0, SEEK_SET) == 0 && (pff_buf[i] = malloc((size_t)n)) && fread(pff_buf[i], 1, (size_t)n, f) == (size_t)n) pff_len[i] = (unsigned)n;
        else { free(pff_buf[i]); pff_buf[i] = NULL; pff_len[i] = 0; } if (f) fclose(f); }
    __atomic_store_n(&pff_busy, 0, __ATOMIC_RELEASE); LightEvent_Signal(&pff_done); }
static void pff_wait(void) { while (__atomic_load_n(&pff_busy, __ATOMIC_ACQUIRE)) LightEvent_Wait(&pff_done); }
void pm_prefetch_files_release(void) { pff_wait(); for (int i = 0; i < PFF_N; i++) { free(pff_buf[i]); pff_buf[i] = NULL; pff_len[i] = 0; pff_path[i][0] = 0; } }
static int pf_start(void); void pm_prefetch_files(const char* shape) { static const char* const ext[PFF_N] = { "nds", "seam", "dlc" }; pm_prefetch_files_release(); if (!pf_start()) return;
    for (int i = 0; i < PFF_N; i++) snprintf(pff_path[i], sizeof pff_path[i], "romfs:/shape/%s.%s", shape, ext[i]);
    LightEvent_Clear(&pff_done); __atomic_store_n(&pff_busy, 1, __ATOMIC_RELEASE); LightEvent_Signal(&pf_go); }
FILE* pm_pf_fopen(const char* path, const char* mode) { /* a posted path from memory (waits for the job), else the file */
    for (int i = 0; i < PFF_N; i++) if (pff_path[i][0] && strcmp(path, pff_path[i]) == 0) { pff_wait(); if (pff_buf[i]) { pm_xf.pff_n++; return fmemopen(pff_buf[i], pff_len[i], mode); } break; }
    return fopen(path, mode); }
```
## Gate (1 build, 2 runs and the bases). Build, nd_host_test.sh (timeout 600000) PASS, c0gate 0/0/0. Bases `--commit $(git merge-base HEAD main)`: xfer_map_shots, spot_mac01_drec.
- Run both and xfer_map_steps (`--row MTX2 --why "MTX2 file jobs"`). Pass: both cmp N/N byte-equal, `xfer4 map n1` ends with `pf 3`, xfer4_check OK.
- capture_check `--row MTX2 --proof "xfer4 map n1"` with the MTX0 capture words: CAPTURE CHECK OK with `pf 3`. pf 0: SendMessage the oracle the log path.
- Fix only your own edits. Any DIFF or fault: SendMessage the oracle the output path. Fallback: delete the hook (step 3), the files load on L as now.
- One commit, then delete this page. SubagentHandback: hash, both cmp lines, the xfer4 and xfer2 lines, CAPTURE line.
