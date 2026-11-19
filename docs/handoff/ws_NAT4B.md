# ws_NAT4B (Haiku): epchk detail: for each disagreement, the unit id, the direction, the core 1 rule code and the core 0 skip inputs. Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main after `Merge NAT4A`. Worktree build3ds/wt/NAT4B, branch NAT4B. PM_ROW=NAT4B PM_LANE=NAT4B. No other row edits these files now.
Edit only: 3ds/src/nd_rec.c (NAT4A line 415), 3ds/src/nd_walk.c (NAT4A lines 233-238, 312), 3ds/src/pm_gpu.c (the epchk line 635).
## Facts (NAT4A c0c1c37478)
- FACT epchk: spot_mac01 units 8928 bad 168, entcape_kmr03 17674 bad 164, fxparts 40680 bad 1024. A W3 cut from core 1 is byte-equal only with bad 0, so the rule needs data first.
- FACT the core 0 skip test (nd_unit_same nd_set.c:123-147) reads sc_ok for OMH, PRIM, ENV, TEXTURE, COMB, sc_sig_now() == sc_sig for TEXTURE, COMB, and box_folded, cmb_gen_seen == comb_gen for COMB.
  The core 1 rule (nd_ds_ep_rule nd_ds.c:402) models them as sc_lost and sc_dirty (nd_ds.c:373-387) and returns a reason code (1 change, 2 lost or dirty sync, 6 rare, 7 MW, 8 rect or misc).
## Edits (new and changed lines end with `/* NAT4B */`)
1. nd_rec.c line 415: the mark carries the rule code in w1. The line becomes
   `    if (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB) { const unsigned rc = nd_ds_ep_rule(w0, ch, lost, dirty); if (rc) st_put(ND_FS_W0(ND_FS_MISC, ND_FS_MISC_EP << 16), rc | (lost ? 0x100u : 0u) | (dirty ? 0x200u : 0u) | (ch & 1u) << 10); } /* NAT4B: the cut and its reason */`
2. nd_walk.c line 233: add `static uint32_t ep_mw; static unsigned epk_h[64][2], epk_nl;` to the line. Line 312 becomes `        case ND_FS_MISC_EP: ep_mark = 1; ep_mw = b; nd_epk_mk++; break; /* NAT4B: the mark and its reason word */`.
   Above nr_set add:
```c
static __attribute__((cold, noinline)) void epk_note(unsigned id, int same, const uint32_t* u) { /* NAT4B: one disagreement */
    extern unsigned sc_sig, comb_gen, cmb_gen_seen; unsigned sc_sig_now(void); extern int box_folded;
    const unsigned dir = same ? 1u : 0u; /* 0: core 0 cuts with no mark, 1: a mark and core 0 skips */
    epk_h[id & 63u][dir]++;
    if (epk_nl < 24u && pm_drec_now()) { epk_nl++; pm_log_evt("epbad f%u id %u dir %u mw %03x sc_ok %d sig %d gen %d fold %d a %06x b %08x", pm_frame_count(), id, dir, ep_mark ? (unsigned)ep_mw : 0u,
        sc_ok, sc_sig_now() == sc_sig, cmb_gen_seen == comb_gen, box_folded, (unsigned)(u[0] & 0xffffffu), (unsigned)u[1]); }
}
void nd_epk_log(void) { for (unsigned i = 0; i < 64u; i++) for (unsigned d = 0; d < 2u; d++) if (epk_h[i][d]) pm_log_evt("epbin id %u dir %u n %u", i, d, epk_h[i][d]); } /* NAT4B */
```
   In nr_set (line 237): `if ((same == 0) != (ep_mark != 0)) nd_epk_bad++;` becomes `if ((same == 0) != (ep_mark != 0)) { nd_epk_bad++; epk_note(i_, same, u); }`. Call epk_note before `ep_mark = 0;` (it reads ep_mark).
   (pm_drec_now: pm_gpu.c, add its prototype `int pm_drec_now(void);` above epk_note when nd_walk.c lacks it.)
3. pm_gpu.c line 635: after the epchk pm_log_evt, inside its braces, add `void nd_epk_log(void); nd_epk_log(); /* NAT4B */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4b_host.txt 2>&1`: PASS. Commit `NAT4B 1: epchk detail`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4b_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4B --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT4B: epchk detail" > build3ds/out/nat4b_loop.txt 2>&1`.
4. `grep -H "epchk\|epbad\|epbin\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4B_cmp_*.txt > build3ds/out/nat4b_epchk.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4B/pm_3ds.elf > build3ds/out/nat4b_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate `calls 122 interp 34 state 23`. nat4b_epchk.txt holds epbin lines. Fix rights: compile errors, missing externs. A DIFF or a tool fault: stop, send nat4b_epchk.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4B 2: gate. 3 drec equal; epbin lines` with `git add -f build3ds/out/nat4b_epchk.txt build3ds/out/nat4b_loop.txt`. It deletes this page. One SubagentHandback: hash, all epbin lines, the first 8 epbad lines of each bench.
