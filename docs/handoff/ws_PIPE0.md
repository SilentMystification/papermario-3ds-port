# ws_PIPE0 (Haiku): fence census: the first draw index per frame of each event a late frame begin must wait for (prof line `pipe`)
Designer: oracle NATIVE19 (plan_pipe.md 3). Worktree build3ds/wt/PIPE0, branch PIPE0. PM_ROW=PIPE0 PM_LANE=PIPE0. Start: main 4a89ac5db7 or later; grep the quoted text.
Why Haiku: one note function, 9 one-line calls and one print line, given below. No draw change. Indentation: as in the file.
FACT: draws_frame (3ds/src/pm_gpu.c:103) counts the native submits of the frame. The draw order is the same on Azahar and the console, so the Azahar values decide.
Objective: `prof f<F> w<W> pipe fr <frames> draws <mean> kick <mean>/<n> ring <mean>/<n> tex <mean>/<n> split <mean>/<n> off <mean>/<n> room <mean>/<n>`.
<mean> = mean first draw index (draws_frame when the event did not occur), <n> = frames where it occurred. Counter: none; it decides PIPE2 and PIPE3.
## Edits (CRLF files: use the Edit tool)
1. pm_gpu.c:846. After `static void kick_split(void);` add:
   `enum { PP_KICK, PP_RING, PP_TEX, PP_SPLIT, PP_OFF, PP_ROOM, PP_N }; /* PIPE0: fence kinds (plan_pipe.md 3) */`
   `static unsigned pp_first[PP_N] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u }, pp_sum[PP_N + 1], pp_hit[PP_N], pp_fr;`
   `void pm_pipe_note(unsigned k) { if (k < PP_N && pp_first[k] == ~0u) pp_first[k] = draws_frame; } /* PIPE0 */`
   `static void pp_frame_end(void) { for (unsigned k = 0; k < PP_N; k++) { const int h = pp_first[k] != ~0u; pp_sum[k] += h ? pp_first[k] : draws_frame; pp_hit[k] += (unsigned)h; pp_first[k] = ~0u; } pp_sum[PP_N] += draws_frame; pp_fr++; }`
   `void pm_gpu_pipe_take(unsigned o[14]) { for (unsigned k = 0; k < PP_N; k++) { o[k] = pp_sum[k]; o[6 + k] = pp_hit[k]; pp_sum[k] = 0; pp_hit[k] = 0; } o[12] = pp_sum[PP_N]; o[13] = pp_fr; pp_sum[PP_N] = 0; pp_fr = 0; }`
2. Notes (pm_gpu.c; the kind number in brackets):
   - :853 `    kick_done = 1;` -> `    kick_done = 1; pm_pipe_note(PP_KICK);`
   - :1385, after the line `        const void* bb = (ndt_mdl || nb.gi_kind == 1) ? (const void*)nb.rchunk[0].vtx : (const void*)nb.gring;` add
     `        if (!(ndt_mdl || nb.gi_kind == 1)) pm_pipe_note(PP_RING); /* PIPE0: a walker ring draw */`
   - first line inside `void split_frame(void) {` (:304), `static void sr_split(void) {` (:2521), `static int grab_impl(const void* cfb, int y0, int y1, int prev_frame) {` (:2832):
     `    pm_pipe_note(PP_SPLIT); /* PIPE0 */` (cmd_split_forced :1546 calls split_frame: no note there)
   - first line inside `int pm_gpu_off_begin(const void* base, int stride, int siz, int band_h) {` (:2229): `    pm_pipe_note(PP_OFF); /* PIPE0 */`
   - :2645 after `    if (q->numEntries < 24) return;` add `    pm_pipe_note(PP_ROOM); /* PIPE0 */`; first line inside `void pm_gx_drain(void) {` (:2659): the same line.
   - :3299 before `    kc_build(); /* INTERPB3: ...` add `    pp_frame_end(); /* PIPE0 */`
3. 3ds/src/pm_tex.c: near the top (after the includes) add `void pm_pipe_note(unsigned k); /* pm_gpu.c PIPE0 */`. Notes with kind 2 (PP_TEX):
   - first line inside `static __attribute__((noinline)) int load_miss(` (:1412, before `src_native = pm_fb_native(img);` at :1415): `    pm_pipe_note(2u); /* PIPE0: PP_TEX */`
   - :342, before `    cell_key[c] = (unsigned short)key; cell_ph[c] = ph; cell_fr[c] = tc_frame;`: `    pm_pipe_note(2u); /* PIPE0: a glyph cell is filled */`
4. 3ds/src/pm_phase.c:643-644. After the c0s print (the statement that ends with `C1(u10[6])); }`) add:
   `            { unsigned pp[14]; extern void pm_gpu_pipe_take(unsigned o[14]); pm_gpu_pipe_take(pp); const unsigned f_ = pp[13] ? pp[13] : 1u; /* PIPE0 */`
   `              pm_log_prof_line("prof f%u w%u pipe fr %u draws %u kick %u/%u ring %u/%u tex %u/%u split %u/%u off %u/%u room %u/%u", pf_total, pf_w, pp[13], pp[12] / f_,`
   `                  pp[0] / f_, pp[6], pp[1] / f_, pp[7], pp[2] / f_, pp[8], pp[3] / f_, pp[9], pp[4] / f_, pp[10], pp[5] / f_, pp[11]); }`
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=PIPE0 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row PIPE0`: byte-equal.
3. `python 3ds/tools/capture_check.py --row PIPE0 --proof "pipe fr" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   Never add phase=on. Copy the last 2 `pipe` lines into the hand-back (these values count: the draw order is deterministic).
Fix rights: compile errors and declaration placement. A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `PIPE0 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line> <pipe line>` deletes this page. Hand-back: one SubagentHandback: commit hash, results.
