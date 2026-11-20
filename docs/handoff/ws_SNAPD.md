# ws_SNAPD (Haiku: closed edits, byte-equal): a persistent core 1 cache of the combine decision of a snap (key: combine inputs, mode5, vprim)
Designer: oracle NATIVE15 (plan_perf.md F1). Branch from main. Worktree build3ds/wt/SNAPD, branch SNAPD. PM_ROW=SNAPD PM_LANE=SNAPD.
Facts: the snap intern is per context, so each frame stores its unique snaps again (PERF1 gate: spot new 10126 hit 9446 at f201; fxparts new 57877
hit 46675). Every new snap runs the decision: nd_pso_find_i (binary search), nd_pso_fix or nd_comb_core1, pm_comb_colours_k (nd_snap.c:63-67).
The decision reads only r.in, sn_mode5 and sn_vprim (nd_snap.c:63-67, nd_decide.c:232-248): a cache of its output is exact.
Objective: core 1 decides once per unique combine input over many frames. c0gate stays 0 / 0 / 1. Proxy (Azahar): `dec` (decisions run) falls far below `new`.
Edit only nd_snap.c (line numbers main c23333bb73): the includes (:5-16), :25-28, :60, :63-67. Source is CRLF.
## Edits
1. Add `#include <stdlib.h>` after `#include <string.h>`.
2. After :25 add:
   `typedef struct { uint32_t h; int32_t m5; uint32_t vp; int32_t key; NdCombIn in; NdCombOut o; } SdEnt; /* SNAPD: one decision */`
   `#define SD_N 32u /* SNAPD: entries (a power of 2), about 17 KB on the heap at first use */`
   `static SdEnt* sd_t; static unsigned char sd_tried; unsigned nd_sd_new, nd_sd_hit;`
3. si_log (:28): the format gets ` dec %u dhit %u` at the end and the arguments `, nd_sd_new, nd_sd_hit`.
4. :60 before: `const int m5 = sn_mode5; const uint32_t h = si_hw(si_hw(si_hw(2166136261u ^ (uint32_t)m5 ^ sn_vprim << 8, &r.in, sizeof r.in), &r.role, 24u), &r.plod, ...);`
   after: `const int m5 = sn_mode5; const uint32_t hi = si_hw(2166136261u ^ (uint32_t)m5 ^ sn_vprim << 8, &r.in, sizeof r.in);`
   `const uint32_t h = si_hw(si_hw(hi, &r.role, 24u), &r.plod, sizeof r - offsetof(NdSnapChk, plod)); /* SNAPD: hi is the decision key */`
5. :63-67 (`r.key = nd_pso_find_i(...)` to the pm_comb_colours_k line) become:
   `if (!sd_t && !sd_tried) { sd_tried = 1; sd_t = (SdEnt*)calloc(SD_N, sizeof *sd_t); } /* SNAPD: NULL keeps the plain decision */`
   `SdEnt* const de = sd_t ? &sd_t[hi & (SD_N - 1u)] : NULL;`
   `r.plan_on = -1;`
   `if (de && de->h == hi && de->m5 == m5 && de->vp == sn_vprim && !memcmp(&de->in, &r.in, sizeof r.in)) { nd_sd_hit++; r.key = de->key; o = de->o; }`
   `else { r.key = nd_pso_find_i(&r.in, sn_vprim, &o);`
   `    if (r.key >= 0) nd_pso_fix(&o, &r.in, sn_mode5);`
   `    else { nd_snm++; nd_comb_core1(&r.in, sn_vprim, sn_mode5, &o); } /* NDR0D */`
   `    if (o.plan_on) pm_comb_colours_k(&o.plan, r.in.prim, r.in.env, r.in.prim_lo8 & 0xffu, r.in.key_c, r.in.key_s, r.in.conv_k4 & 255u);`
   `    nd_sd_new++; if (de) { de->h = hi; de->m5 = m5; de->vp = sn_vprim; de->key = r.key; de->in = r.in; de->o = o; } }`
   Keep :68 onward (`r.o = o;` ...) unchanged.
## Checklist (hw_guidelines.md)
- No per-frame allocation (one calloc for the life of the game). Direct-mapped table: one probe, no scan. Word hash (the PERF1 hash), no division, no double.
- The entry copy (about 520 B) replaces a binary search over the pso keys, the plan stage copies and the colour work.
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/snapd_host.txt` (timeout 600000): PASS. Commit `SNAPD 1: decision cache`.
2. `PM_LANE=SNAPD sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1. memcheck line of the build: OK.
3. Base, run and cmp as ws_MDLBLK step 3, benches spot_mac01_drec,fxparts_drec: every shot byte-equal.
   Pass also: the last snapi line of each log has new + hit = snn and dec + dhit = new.
Fix rights: the edited lines and compile errors. A DIFF, a broken sum or a crash: stop, send the output path to oracle NATIVE15.
Stop: last commit `SNAPD 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, memcheck, cmp, the last snapi line of each run.
