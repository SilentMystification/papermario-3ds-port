# ws_NAT1E (Haiku): log the first 16 c0diff cases with their detail (which decision field differs, which command draws). Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main (04d851951c or later). Worktree build3ds/wt/NAT1E, branch NAT1E. PM_ROW=NAT1E PM_LANE=NAT1E. Runs alone or with NAT2D round 2 (no shared hunk: NAT2D edits nd_snap.c lines 67-80, 198, 208).
Edit only: 3ds/src/nd_snap.c (line 232 and 1 new line above it), 3ds/src/pm_gpu.c (the c0_dwho line 603). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main 04d851951c)
- FACT NAT1D c0diff: wdiff 12 (spot_mac01), 4 (entcape_kmr03), 113 (fxparts). Keys: 0014b5c0 (nd_nr_run, the stream state runs: 12, 4, 42) and 0014ba00 (nd_parts_run_v: 71).
  The keys follow the NAT1B order nd_nr_run < nd_parts_run_v < nd_ntab_rec (nd_walk.c:347, :455, :563). So NAT3A and NAT3D would change these draws: they WAIT for this data.
- FACT nd_snap_cdiff (nd_snap.c:232) compares plan_on, the plan bytes and leg with sc_cur.o. c0_dwho (pm_gpu.c:603) counts a draw whose last walker writer differs. c0_wk is the decision's command (pm_gpu.c:600).
## Edits (new lines end with `/* NAT1E */`)
1. nd_snap.c, a new line above line 232: `unsigned nd_cd_w, nd_cd_s, nd_cd_off; /* NAT1E: walker and snap words of the last compare (plan_on << 8 | leg), the first differing plan byte (0xffff none) */`
   Line 232 becomes:
```c
int nd_snap_cdiff(int plan_on, int leg, const PmPlan* plan) { const NdCombOut* s = &sc_cur.o; /* NAT1E: the detail words below */
    nd_cd_w = (unsigned)plan_on << 8 | ((unsigned)leg & 0xffu); nd_cd_s = (unsigned)s->plan_on << 8 | ((unsigned)s->leg & 0xffu); nd_cd_off = 0xffffu;
    if (plan_on && s->plan_on) { const unsigned char* a = (const unsigned char*)plan; const unsigned char* b = (const unsigned char*)&s->plan; for (unsigned i = 0; i < sizeof *plan; i++) if (a[i] != b[i]) { nd_cd_off = i; break; } }
    return s->plan_on != plan_on || (plan_on ? nd_cd_off != 0xffffu : s->leg != leg); }
```
2. pm_gpu.c line 603: in c0_dwho, right after `c0_wd++;` insert:
   ` { extern unsigned nd_cd_w, nd_cd_s, nd_cd_off, pm_frame_count(void); static unsigned nc; if (nc < 16u) { nc++; const uint32_t* s = nd_stream_cur; pm_log_evt("c0dcase f%u wk %08x dk %08x c0d %u w %03x s %03x off %u sz %u", pm_frame_count(), (unsigned)c0_wk, !s ? 0u : (s[0] >> 24) == 10u ? (unsigned)s[1] : (unsigned)(s[0] >> 24), (unsigned)nd_c0d, nd_cd_w, nd_cd_s, nd_cd_off, (unsigned)sizeof(PmPlan)); } } /* NAT1E: dk = the command at the draw */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat1e_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT1E 1: c0dcase lines`.
2. For B in spot_mac01_drec, fxparts_drec: `PM_AZT_BASE_SLOT=2 python 3ds/tools/bench.py base <B> > build3ds/out/nat1e_base_<B>.txt 2>&1` (slot 2: a fresh slot 1 Azahar fails its warm restart).
3. `sh 3ds/tools/rowloop.sh --row NAT1E --bench spot_mac01_drec,fxparts_drec --no-host --why "NAT1E: c0dcase detail" > build3ds/out/nat1e_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|c0dwho\|c0dcase" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/nat1e_c0src.txt`.
5. Names of every wk and dk key: as ws_NAT1B step 5, with `re.findall(r'(?:wk|dk) (\w+)', ...)` on nat1e_c0src.txt and the lane ELF build3ds/lane-NAT1E/pm_3ds.elf, into build3ds/out/nat1e_who.txt.
6. `python 3ds/tools/c0gate.py build3ds/lane-NAT1E/pm_3ds.elf > build3ds/out/nat1e_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. nat1e_c0src.txt holds c0dcase lines (16 or fewer per bench).
Fix rights: compile errors. A DIFF or a tool fault: stop, send the output path to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT1E 2: gate. 2 drec equal; c0dcase lines` with `git add -f build3ds/out/nat1e_c0src.txt build3ds/out/nat1e_who.txt build3ds/out/nat1e_loop.txt`. It deletes this page. One SubagentHandback: hash, the c0dcase lines with the key names.
