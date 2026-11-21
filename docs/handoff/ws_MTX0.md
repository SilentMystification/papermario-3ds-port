# ws_MTX0: map transition step census (Haiku, oracle NATIVE22, plan_maptrans.md 9)
Objective: per transition, print the steps by phase and the 10 longest steps with their stage counts. No behaviour change. It is the measurement base
of MTX1 and MTX2, not the fix. Counter: none of C1 to C12. New metric: xfer4 phase max (o fade out, c covered, n fade in, t tail). Use the Edit tool. Edits in directory order:
1. 3ds/include/pm_port.h:399: after `unsigned cap_cpu;` add `unsigned rst_b; struct { const void* k; unsigned n, t; } dc[4]; /* MTX0 */`.
2. 3ds/src/nd_walk.c:476 (after `pm_wq_idle();`): copy the DRAINC block :477-480 into `if (pm_xf.on)`, with `pm_xf.dc`, slots 0 to 2 named, slot 3 the rest (key 0), time `pm_ticks() - xt0`.
3. 3ds/src/pm_mapdata.c:126 (after the for loop, before the dataprobe test): `pm_xf.rst_b += bd + bz; /* MTX0 */`.
4. src/heap.c:12-15. Before: `void* p = _heap_malloc(&heap_generalHead, size);` then `return p;`. After: the same 2 lines with
   `{ extern unsigned pm_hp_a; pm_hp_a++; } /* MTX0 */` between them. Do the same with pm_hp_f in general_heap_free (:21-24).
5. 3ds/src/pm_xfer.c:24: add `unsigned pm_hp_a, pm_hp_f; extern unsigned pm_wq_drw; /* MTX0 */`. Add before pm_xfer_step (:178):
```c
enum { XV_DRN, XV_DRW, XV_DRT, XV_AU, XV_DCN, XV_DCU, XV_RST, XV_ROM, XV_ROMB, XV_YAY, XV_TEX, XV_MDL, XV_DLC, XV_HPA, XV_HPF, XV_N }; /* MTX0 */
typedef struct { unsigned s, w, v[XV_N]; char ph; } XfRow; static XfRow xr_top[10], xr_p; static unsigned xr_n, xp_n[4], xp_max[4], xp_o34[4];
static void xr_snap(XfRow* c) { unsigned n, us, mx; pm_dspa_dec_read(&n, &us, &mx);
    const unsigned v[XV_N] = { pm_xf.dr_n, pm_wq_drw, pm_xf.dr_t, pm_xf.au_t, n, us, pm_xf.rst_b, pm_xf.rom_n, pm_xf.rom_b, pm_xf.yay_n, pm_xf.tex_n, pm_xf.mdl_miss, pm_xf.dlc_n, pm_hp_a, pm_hp_f };
    memcpy(c->v, v, sizeof v); }
static void xr_step(unsigned w, int ld) { /* MTX0: the phase and the deltas of one step, the 10 longest steps */
    u8 t; f32 z; get_screen_overlay_params(SCREEN_LAYER_FRONT, &t, &z);
    const int cov = (t == OVERLAY_SCREEN_COLOR || t == OVERLAY_VIEWPORT_COLOR) && z >= 255.0f;
    const int p = (ld || cov || (gOverrideFlags & GLOBAL_OVERRIDES_DISABLE_DRAW_FRAME)) ? 1 : !xf_loaded ? 0 : get_game_mode() == GAME_MODE_CHANGE_MAP ? 2 : 3;
    XfRow c, d; xr_snap(&c); for (unsigned k = 0; k < XV_N; k++) d.v[k] = c.v[k] - xr_p.v[k];
    xr_p = c; d.s = xf_steps; d.w = w; d.ph = "ocnt"[p];
    xp_n[p]++; if (w > xp_max[p]) xp_max[p] = w; if (w > T34) xp_o34[p]++;
    unsigned i = xr_n < 10u ? xr_n++ : 10u;
    if (i == 10u) { if (w <= xr_top[9].w) return; i = 9u; }
    while (i > 0u && xr_top[i - 1u].w < w) { xr_top[i] = xr_top[i - 1u]; i--; }
    xr_top[i] = d; }
```
6. pm_xfer.c:203: insert `xr_step(w, ld);` before the xf_print test. pm_xfer.c:115 (xf_begin): reset xr_top, xr_n, xp_n, xp_max, xp_o34 to 0, then `xr_snap(&xr_p);`.
7. pm_xfer.c:101 (before `xf_kind = XF_NONE;`), model the xfer3 print :99-100, at most 16 args a line. Per top row: `xfer5 %s t%u s%u %c w %s dr %u/%u/%s au %s dec %u/%u rst %u`
   (w, DRT, AU by msf, then DCU / 1000u, RST >> 10) and `xfer6 %s t%u rom %u/%u yay %u tex %u mdl %u dlc %u hp %u/%u` (ROMB >> 10). Then `xfer7 %s n%u drc` and
   4 times ` %08x %u %s` (dc k, n, msf t). Last: `xfer4 %s n%u | o %u/%s/%u c %u/%s/%u n %u/%s/%u t %u/%s/%u` (xp_n, msf xp_max, xp_o34 of each phase).
8. 3ds/tools/azt.d/MTX0.json: scenario xfer_map_steps = xfer_map_mac01 (azt_scenarios.json) with until and event `xfer4 map n1 \\|`. 3ds/tools/bench.d/MTX0.json:
   bench xfer_map_steps, kind azahar_stream, tags azahar xfer, gates [], log_metric xf4_c_max `xfer4 map n\\d+ \\| o \\d+/[\\d.]+/\\d+ c \\d+/([\\d.]+)`.
9. 3ds/tools/xfer4_check.py (30 lines or fewer): per `xfer map ... f<N>` line and its xfer4 line, the o+c+n+t step sum must be N. It prints
   `xfer4 check OK <transitions>` or `xfer4 check FAIL n<k> f<N> sum <S>`. The switch `--seed` adds 1 to the first N and must print FAIL.
## Gate (1 build, 2 runs and the base, budget 8 Azahar starts). Build, `sh 3ds/tools/nd_host_test.sh` (timeout 600000), c0gate calls 0 interp 0 state 0. `bench.py base xfer_map_shots --commit $(git merge-base HEAD main) --row MTX0`.
- Run xfer_map_shots and xfer_map_steps (`--row MTX0 --why "MTX0 census"`). Pass: cmp xfer_map_shots N/N byte-equal. xfer4_check OK, and FAIL with `--seed`.
- `capture_check.py --row MTX0 --proof "xfer4 map n1" --profile prof warp=mac_01:0 story=ch0_toad_town pos=0:0:200 padscript=go_0_700_300,wait150 profdisk=1`: CAPTURE CHECK OK.
- Fix only your own edits. A xfer_map_shots base fault: use spot_mac01_drec and note it. Any other DIFF or fault: SendMessage the oracle the output path.
- One commit (code, 2 json files, the tool), then delete this page. SubagentHandback: hash, cmp line, xfer4 line, first 3 xfer5 and xfer6 lines, check lines, CAPTURE line.
