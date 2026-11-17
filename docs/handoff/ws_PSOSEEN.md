# ws_PSOSEEN (Haiku): log each pso table miss key once, add the seen keys as descs (stage S0 of plan_walker_delete.md)
Designer: oracle PHASEE (SendMessage). Worktree build3ds/wt/PSOSEEN, branch PSOSEEN from main, PM_ROW=PSOSEEN PM_LANE=PSOSEEN.
Edit only: 3ds/src/nd_comb.c, new 3ds/tools/psoseen.py, new 3ds/src/nd_pso_seen.c. Waits for nothing. Model: Haiku (spec below).
## Objective. No counter moves. Stage S1 (PSOFB) needs few misses. The miss count after this row is OPEN.
- FACT: every combine decision passes nd_comb_pick (nd_comb.c:240). A miss runs nd_comb_core (:241). fxslot_cap (6ff72ad823): `lk 5185
  miss 26091 lkbad 0`. gen_pso_tab.py makes keys from every ND_PSO_DESC in src and 3ds/src (build.sh:190): a desc of a missed key gives a hit.
## Edits
1. nd_comb.c: add above nd_comb_pick (:240) with the comment `/* PSOSEEN: one log entry per distinct missed key (256 at most) */`.
```c
static __attribute__((cold, noinline)) void pso_seen_note(const NdCombIn* in) {
    static uint32_t seen[256]; static unsigned n; uint32_t h = 2166136261u;
    if ((unsigned)(in->tex_on | in->use_t1 | in->t1_valid | in->dual_on | in->box_prod | in->box_folded | in->comb_set | in->comb_alpha_shade) > 1u) return;
    const uint32_t k[9] = { in->w0, in->w1, in->omode_h, in->prim_lo8 & 0xffu, in->key_c, in->key_s, in->conv_k4, (uint32_t)pm_comb_vprim,
        nd_pso_flags(in->tex_on, in->use_t1, in->t1_valid, in->dual_on, in->box_prod, in->box_folded, in->comb_set, in->comb_alpha_shade) };
    for (unsigned i = 0; i < 9u; i++) h = (h ^ k[i]) * 16777619u;
    for (unsigned i = 0; i < n; i++) { if (seen[i] == h) return; }
    if (n >= 256u) return;
    seen[n++] = h;
    pm_log_evt("psoseen 0x%x, 0x%x, 0x%x, 0x0, 0x0, 0x%x, 0x%x, 0x%x, %d, %d, %d, %d, %d, %d, %d, %d, 0, %u, 0, %u", (unsigned)in->w0, (unsigned)in->w1,
        (unsigned)in->omode_h, (unsigned)in->key_c, (unsigned)in->key_s, (unsigned)in->conv_k4, in->tex_on, in->use_t1, in->t1_valid, in->dual_on,
        in->box_prod, in->box_folded, in->comb_set, in->comb_alpha_shade, (unsigned)pm_comb_vprim, (unsigned)(in->prim_lo8 & 0xffu));
}
```
   Field order: NdPsoDesc (nd_api.h:27-35). :241 before `{ pk_miss++; nd_comb_core(`, after `{ pso_seen_note(in); pk_miss++; nd_comb_core(`.
2. psoseen.py `<log>...`: take the text after `psoseen ` of each line. Read the ND_PSO_DESC lines of 3ds/src/nd_pso_seen.c if it exists.
   Merge, drop equal field texts, sort. Write: `/* nd_pso_seen.c: written by 3ds/tools/psoseen.py (PSOSEEN). Missed pso keys. */`, `#include
   "nd_api.h"`, `#pragma GCC diagnostic ignored "-Wunused-const-variable"`, one line per key `ND_PSO_DESC(pso_seen_<crc32 of the field text,
   8 hex>, <field text>);`. Print `psoseen keys <total> new <added>`. `--selftest`: 3 lines (2 equal) in a temp dir give `keys 2 new 2`, else exit 1.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-2. `python 3ds/tools/psoseen.py --selftest`. Commit `PSOSEEN 1: one log entry per missed pso key; psoseen.py`.
2. `sh 3ds/build.sh` (bench.py run does not build). Then for B in spot_mac01_drec wscmb_pause_drec axns_sam2_drec fxchk_a fxchk_b
   fxchk_c: `python 3ds/tools/bench.py run $B --row PSOSEEN --why "PSOSEEN capture"`. Keep each run.log path it prints.
3. `python 3ds/tools/psoseen.py <the 6 run.log paths>`. Commit `PSOSEEN 2: <n> seen keys (nd_pso_seen.c)`. 4. `python 3ds/tools/bench.py
   base spot_mac01_drec --row PSOSEEN`, the same for wscmb_pause_drec (base runs, not charged).
5. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row PSOSEEN --bench spot_mac01_drec,wscmb_pause_drec --why "PSOSEEN gate" > build3ds/out/psoseen_loop.txt 2>&1`.
## Tests that decide. Budget 8 Azahar starts (6 capture, 2 gate).
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal against the merge-base.
- `grep -c "psoseen 0x"` on the gate run.log of spot_mac01_drec (path in build3ds/out/loop_PSOSEEN_run.txt) is lower than on its step 2 run.log.
## Fix rights: only the 3 files. rowloop refuses the 2 benches together (speed): 2 calls, the second --no-host. Else stop, send the path to the oracle.
## Stop: commit `PSOSEEN 3: gate. 2 benches byte-equal; keys <n>; psoseen <a> -> <b>` deletes this page. Hand back hashes, LOOP lines, counts.
