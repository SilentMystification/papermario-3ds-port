# ws_NAT2D (Haiku): a list snap table holds up to 256 ranges over up to 16 unique snaps, so the fxparts lists over 16 ranges get a table. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. lsover lines: 16 to 0.
Designer: oracle NATIVE. Branch from main (b485faae44 or later). Worktree build3ds/wt/NAT2D, branch NAT2D. PM_ROW=NAT2D PM_LANE=NAT2D. Wave W2a with NAT1D.
Edit only: 3ds/src/nd_snap.c (lines 67-77 and 195-205 only), 3ds/include/nd_snap.h (lines 82-89). NAT1D appends to nd_snap.c after line 230 only (40+ lines apart). No edit in nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (main b485faae44)
- FACT NAT2G fxparts: 16 `lsover` lines (frames 464-483 with 120 triangle ordinals, frame 534 with 42). Such a list gets no table (nd_snap.c:76 `ls_ovl`), so core 0 draws it from walker decisions.
- FACT table format today (writer nd_snap.c:77): b[0] = ranges = snaps, b[1] = triangle count, b[2+k] = range word (ord bits 0-29, ND_LS_DUP bit 30, ND_LS_EP bit 31), then the snaps, then the NdDs end state.
- FACT readers: nd_snap_lsr (nd_snap_in.c:85, ND_LS_ORD), nd_lsr_tri (nd_snap.c:200, :205 snap of range r), nd_lsr_end (:195 end state after b[0] snaps), the host test nd_snap_test.c:119-132 (ords below 16, flags only in bits 30-31). b[1] has no reader.
- FACT a table is one nd_rec_data_st block: at most 32744 bytes (nd_rec.c:470: 2 + 2 units <= CH_WORDS - 4, CH_WORDS 8192).
## New format: b[0] = R ranges, b[1] = S unique snaps, b[2+k] = ord (bits 0-15) | snap index << 16 (bits 16-29) | DUP | EP, then S snaps, then the NdDs end state.
## Edits
1. nd_snap.h: line 88 becomes `#define ND_LS_ORD(f) ((f) & 0xffffu) /* NAT2D: ord bits 0-15 */` and a new line after it: `#define ND_LS_SNAP(f) (((f) >> 16) & 0x3fffu) /* NAT2D: the snap index of a range */`.
2. nd_snap.c lines 67-68: `#define LS_MAX 16u` becomes `#define LS_SMAX 16u /* NAT2D: unique snaps */` plus `#define LS_RMAX 256u /* NAT2D: ranges */`.
   Line 68 arrays: `static NdSnapChk ls_s[LS_SMAX], ls_t; static uint32_t ls_f[LS_RMAX]; static unsigned ls_n, ls_sn, ls_ord;` (the rest of line 68 stays). Add after it:
   `_Static_assert(ND_LS_HDR(LS_RMAX) + LS_SMAX * sizeof(NdSnapChk) + ND_LS_DSB <= 32744u, "NAT2D: a table fits one nd_rec_data_st block");`
   `static unsigned ls_find(void) { for (unsigned i = 0; i < ls_sn; i++) if (!memcmp(&ls_s[i], &ls_t, sizeof ls_t)) return i; if (ls_sn >= LS_SMAX) return ~0u; ls_s[ls_sn] = ls_t; return ls_sn++; } /* NAT2D */`
3. nd_lsc_begin (line 71): add `ls_sn = 0;` next to `ls_n = 0;`. nd_lsc_unit (line 73), the `if (ls_chg) { ... }` block becomes:
```c
        if (ls_chg) { ls_chg = 0;
            if (ls_n >= LS_RMAX || ls_ord > 0xffffu) ls_ovl = 1;
            else if (snap_build(&nd_ds, 0, &ls_t)) { const unsigned k = ls_find(); if (k == ~0u) ls_ovl = 1; else { ls_f[ls_n++] = ls_ord | k << 16 | (ls_ep ? ND_LS_EP : 0u); sn_cmds = nd_rec_cmds(); } }
            else if (ls_ep && ls_n) { ls_f[ls_n] = ls_ord | (ls_f[ls_n - 1u] & 0x3fff0000u) | ND_LS_EP | ND_LS_DUP; ls_n++; }
            ls_ep = 0; }
```
4. nd_lsc_end_at line 77 before: `const unsigned h = ND_LS_HDR(ls_n); ... nd_rec_data_st(h + ls_n * (unsigned)sizeof ls_s[0] + ND_LS_DSB); b[0] = ls_n; b[1] = ls_ord; ...`
   after: the size uses `ls_sn * (unsigned)sizeof ls_s[0]`, `b[1] = ls_sn;`, the snap copy is `memcpy((char*)b + h, ls_s, ls_sn * sizeof ls_s[0]);`, the end state goes to `(char*)b + h + ls_sn * sizeof ls_s[0]`.
5. nd_lsr_end (line 195): `+ lr_b[0] * sizeof sc_cur` becomes `+ lr_b[1] * sizeof sc_cur`. nd_lsr_tri line 205: `+ r * sizeof sc_cur` becomes `+ ND_LS_SNAP(lr_b[2u + r]) * sizeof sc_cur`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2d_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT2D 1: list tables of 256 ranges, 16 unique snaps`.
2. For B in fxparts_drec, spot_mac01_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2d_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2D --bench fxparts_drec,spot_mac01_drec --no-host --why "NAT2D: list tables over 16 ranges" > build3ds/out/nat2d_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0who\|lsover\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/fxparts_drec.txt build3ds/azt/logs/spot_mac01_drec.txt build3ds/out/loop_NAT2D_cmp_*.txt > build3ds/out/nat2d_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2D/pm_3ds.elf > build3ds/out/nat2d_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Build rc 0 (the _Static_assert holds). Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. Report the lsover count of fxparts (expected 0).
Fix rights: compile errors. A failed _Static_assert: set LS_RMAX to 128, then 64. A DIFF, a host check FAIL or a tool fault: stop, send build3ds/out/nat2d_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2D 2: gate. 2 drec equal; lsover <n>` with `git add -f build3ds/out/nat2d_c0src.txt build3ds/out/nat2d_loop.txt`. It deletes this page. One SubagentHandback: hash, LS_RMAX used, the lsover count, the 2 c0src lines.
