# ws_PSOFB (Haiku): pso table miss draws a fixed fallback; nd_decide.c leaves the device (stage S1; C9 model -2)
Designer: oracle PHASEE2. WAIT: branch from main only after the PSOSEEN merge (pso_seen_note in nd_comb.c, 3ds/src/nd_pso_seen.c).
Worktree build3ds/wt/PSOFB, branch PSOFB. PM_ROW=PSOFB PM_LANE=PSOFB. Edit only the files of the edits below and 3ds/src/nd_pso_seen.c.
## Facts (code read, main d5325e4e73 and branch PSOSEEN 2dd5cb26e5)
- Device calls into nd_decide.c (FACT, git grep): nd_comb_core (nd_comb.c:241, :246), nd_pso_fix (:243), nd_const_rgb (:77), nd_pso_in_eq
  (nd_psolk.c:29), nd_pso_flags (pso_seen_note). nd_pso_in_eq calls nd_comb_tst. nd_pso_fix calls nd_const_rgb.
- nd_psolk.c references the generated table. The host links of nd_decide.c have no table: the helpers go to a new file, not nd_psolk.c.
- 3ds/CMakeLists.txt:348 globs 3ds/src/*.c: a file moved to 3ds/tools leaves the device build. listcen model: decide, decide_miss (-2).
## Edits
1. `git mv 3ds/src/nd_decide.c 3ds/tools/nd_decide.c`.
2. New 3ds/src/nd_psokey.c: line 1 `/* nd_psokey.c: PSOFB, the device part of the pso decision: the key compare of the table lookup
   (nd_psolk.c) and the constant colour. The decision is host only (3ds/tools/nd_decide.c, gen_pso_tab). */`. Then the include lines
   of nd_decide.c:14-22 without `<math.h>`. Then MOVE (cut, not copy) from 3ds/tools/nd_decide.c: :25-88 (dc_const_in, dc_alpha_k,
   nd_const_rgb), :312-333 (comment, nd_comb_tst), :399-425 (comments, nd_pso_flags, nd_pso_in_eq, nd_pso_fix).
3. Path readers: change `3ds/src/nd_decide.c` to `3ds/tools/nd_decide.c 3ds/src/nd_psokey.c` at build.sh:211 and 3ds/tests/host.d
   coin.sh:6, :8, conv2c.sh:9, decide.sh:5, psolk.sh:12, psotab.sh:10. gen_pso_tab.py:129: `"3ds/src/nd_decide.c",` to
   `"3ds/tools/nd_decide.c", "3ds/src/nd_psokey.c",`. After: `git grep -n "src/nd_decide" -- 3ds` prints nothing.
4. nd_comb.c, above nd_comb_pick, add:
```c
/* PSOFB (user 2026-11-16): a key that the pso table does not hold draws with vertex colour only (legacy mode 0) and logs its key once. */
static __attribute__((cold, noinline)) void pso_fallback(const NdCombIn* in, NdCombOut* o) {
    pso_seen_note(in);
    memset(o, 0, sizeof *o);
    o->use = -1; o->cyc = (int)((in->omode_h >> G_MDSFT_CYCLETYPE) & 3); o->dual = in->dual_on | in->box_prod << 1;
}
```
   nd_comb_pick, before: `{ pso_seen_note(in); pk_miss++; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o); return; }`
   after: `{ pk_miss++; pso_fallback(in, o); return; }`. Delete the block `if (psochk_on()) { NdCombOut r; ... pk_bad++; }`.
   Delete `pk_bad` from its declaration (:220). Delete ` lkbad %u` and its `pk_bad` argument from the 2 psochk format strings.
5. pso_seen_note, the line `if ((unsigned)(in->tex_on | ... ) > 1u) return;`: keep the condition text, change `return;` to
   `{ static int once; if (!once) { once = 1; pm_log_evt("psomiss flags above 1"); } return; }`.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/psofb_host.txt 2>&1`. Commit `PSOFB 1: fixed fallback on a pso miss;
   nd_decide.c to 3ds/tools, device helpers in nd_psokey.c`.
2. `python 3ds/tools/bench.py base spot_mac01_drec,wscmb_pause_drec --row PSOFB --commit $(git merge-base HEAD main)` (not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row PSOFB --bench spot_mac01_drec,wscmb_pause_drec --why "PSOFB gate" > build3ds/out/psofb_loop.txt 2>&1`.
4. `grep -cE "psoseen |psomiss " <each gate run.log of step 3> > build3ds/out/psofb_miss.txt`.
## Tests that decide (1 build, 2 Azahar starts; 1 more round after a fix)
- Host PASS (decide, coin, conv2c, psolk, psotab use the new paths). `git grep -n "nd_comb_core(" 3ds/src` prints nothing.
- `LOOP PASS`: both benches byte-equal. psofb_miss.txt: 0 and 0. `listcen.py` total model 2 lower than main, other totals equal.
## Fix rights: `psoseen ` lines only: `python 3ds/tools/psoseen.py <those run.logs>` (it writes 3ds/src/nd_pso_seen.c), commit, repeat
  step 3 once. Any other failure (psomiss, compile, link, DIFF with count 0): stop, send the output path to oracle PHASEE2. Wait.
## Stop: last commit `PSOFB 2: gate. 2 benches byte-equal; psoseen 0/0; model <n>` deletes this page. One SubagentHandback.
