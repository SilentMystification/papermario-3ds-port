# ws_FALLBK (Haiku): the boot fallback list holds typed units, so the walker stops at its end (fix of the drec variants; counter: none)
Designer: oracle NSTO. Branch from main. Worktree build3ds/wt/FALLBK, branch FALLBK. PM_ROW=FALLBK PM_LANE=FALLBK.
Edit only: 3ds/src/nd_walk.c (pm_gbi_fallback, 3 lines). The NST2B and NST2D branches touch other lines of nd_walk.c.
## Facts (code read, main 5ceee60b7f; WALKSRC log: `run 0x136eb48 n 200000 ra 0x10e84c`, ELF build 6503)
- 0x0136eb48 = `dl` of pm_gbi_fallback (nd_walk.c:770). 0x0010e84c = nd_ops_run. The caller: pm_main.c:464, at each boot frame with no draw.
- nd_walk.c includes no nd_gbi.h (:4-21). So gDPSetCycleType ... gSPEndDisplayList (:791-798) write raw F3DEX2 words (top bytes E3, F7,
  F6, DA, 01, 05, DF). None is a typed id (0x20-0x3F). nd_walk_state (:391) runs each through op_unknown (:397) and finds no NDO_END.
- So nd_ops_run(dl, ND_LIST_MAX, 0) (:801) walks 200000 units (1.6 MB of .bss after dl) each boot frame. On its way it runs pm_f127,
  pm_f255 (pm_gpu.c:1725) and t_blk0 (pm_dspaud.c:52, a system tick) as units. The tick top byte changes per run: 0x39 MTX in one run,
  0x2D COMB and 0x2C RARE (key_s 0x100, sticky) in another. That gives the two drec variants of wscmb_pause_drec (base 86b4a069).
## Edit
1. (round 1: a loop that reads `.words.w0` raises the lkey census of nd_walk.c to 12, listed 11) nd_walk.c: after `#include "nd_lkey.h"`
   (:21) add `#include "nd_gbi.h" /* FALLBK: pm_gbi_fallback's macros write typed units (nd_gx_w, nd_rec.c:487) */`. No loop.
   pm_gbi_fallback (:791-798) is the only gbi macro user in nd_walk.c. nd_gx_w writes the typed unit into a list that is not gMainGfxPos.
2. Do not change the list content or the call at :801.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 tool build, 4 Azahar starts)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fallbk_host.txt`: PASS as on main. Commit `FALLBK 1: typed boot fallback list`.
2. `python 3ds/tools/bench.py base spot_mac01_drec,wscmb_pause_drec --commit $(git rev-parse HEAD) --row FALLBK > build3ds/out/fallbk_base.txt`.
   Pass: both lines `two runs equal, stored`.
3. For each bench, grep the stored log build3ds/basecache/<head hash>/<bench>/run.log (main folder) for `bad dl target`, `outside host memory`
   and `psomiss` > build3ds/out/fallbk_grep.txt. Pass: 0 lines of each.
   Seeded fault (no run): the base logs of 86b4a069 (build3ds/basecache/86b4a069.../<bench>/run.log) hold these lines (FACT: spot_mac01_drec
   lines 25, 28. wscmb_pause_drec lines 127-140). Grep them into the same file as the fault proof.
4. The drec of the head may differ from the 86b4a069 base (the garbage state is gone). This is expected. Do not run bench.py cmp.
## Fix rights: nd_walk.c; compile errors. A remaining grep line, two head runs that differ, a crash: stop, send the output path to oracle NSTO.
Stop: last commit `FALLBK 2: gate. 2 benches two runs equal; bad dl 0, outside host 0, psomiss 0 (base <n>)` deletes this page.
One SubagentHandback: commit hash, the grep counts head and base.
