# ws_NAT2D round 2 (coder a703decd9cfbb581b): each unique snap of a list goes into its own stream block, so a table holds up to 256 ranges over up to 256 unique snaps. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. lsover: 0.
Designer: oracle NATIVE. Continue on branch NAT2D (tip e9a6ea65f5). Worktree build3ds/wt/NAT2D. PM_ROW=NAT2D PM_LANE=NAT2D.
Edit only: 3ds/src/nd_snap.c (the list table writer lines 67-80 and the readers 198, 208). No edit in nd_snap.h, nd_snap_in.c, nd_walk.c, nd_set.c, nd_comb.c, nd_direct.c.
## Facts (branch NAT2D e9a6ea65f5)
- FACT round 1 data: with LS_SMAX 48, fxparts lists at f632-f652 still overflow ("ranges 49 ord 100"). One table is one nd_rec_data_st block of at most 32744 bytes (nd_rec.c:470), and sizeof(NdSnapChk) is 616: at most 49 snaps fit.
- FACT the stream keeps every block until the replay ends (nd_cmd.h:5-6), so a table may point at other stream blocks. nd_rec_data_st writes a JUMP over the data (nd_rec.c:468-478), and core 0 skips it.
- FACT every nd_lsc_begin caller records its command before the table (nd_rec.c:251 entity, :363 parts, nd_gtab.c:139-141 lists). So a data block written during nd_lsc_unit comes after that command.
- FACT readers of the table: nd_lsr_tri (nd_snap.c:208), nd_lsr_end (:198), nd_snap_lsr (nd_snap_in.c:85, reads only b[0] and the range words b[2+k], k < b[0]: no change).
## New format: b[0] = R ranges, b[1] = S snaps, b[2+k] = range word (k < R, as round 1), b[2+R+i] = pointer to snap i (i < S), the NdDs end state at byte ND_LS_HDR(R + S).
## Edits (nd_snap.c, new lines end with `/* NAT2D */`)
1. Lines 67-71 become:
```c
#define LS_SMAX 256u /* NAT2D: unique snaps, each in its own stream block */
#define LS_RMAX 256u /* NAT2D: ranges */
static NdSnapChk ls_t; static const NdSnapChk* ls_p[LS_SMAX]; static uint32_t ls_h[LS_SMAX], ls_f[LS_RMAX]; static unsigned ls_n, ls_sn, ls_ord; static unsigned char ls_chg;
_Static_assert(ND_LS_HDR(LS_RMAX + LS_SMAX) + ND_LS_DSB <= 32744u, "NAT2D: a table fits one nd_rec_data_st block");
static unsigned ls_find(void) { /* NAT2D: the index of a snap equal to ls_t (FNV-1a 32 first), else a new stream block */
    const unsigned char* q = (const unsigned char*)&ls_t; uint32_t h = 2166136261u;
    for (unsigned i = 0; i < sizeof ls_t; i++) h = (h ^ q[i]) * 16777619u;
    for (unsigned i = 0; i < ls_sn; i++) if (ls_h[i] == h && !memcmp(ls_p[i], &ls_t, sizeof ls_t)) return i;
    if (ls_sn >= LS_SMAX) return ~0u;
    NdSnapChk* p = (NdSnapChk*)nd_rec_data_st(sizeof ls_t); *p = ls_t; ls_p[ls_sn] = p; ls_h[ls_sn] = h; return ls_sn++;
}
```
2. nd_lsc_end_at, the line after the `if (!ls_n || ls_ovl) return;` line becomes:
   `    const unsigned h = ND_LS_HDR(ls_n + ls_sn); uint32_t* b = (uint32_t*)nd_rec_data_st(h + ND_LS_DSB); b[0] = ls_n; b[1] = ls_sn; memcpy(b + 2, ls_f, 4u * ls_n);`
   `    for (unsigned i = 0; i < ls_sn; i++) b[2u + ls_n + i] = (uint32_t)(uintptr_t)ls_p[i]; memcpy((char*)b + h, &nd_ds, sizeof nd_ds); /* NCONV2D: the end state */ *dst = (uint32_t)(uintptr_t)b; } /* NAT2D */`
3. nd_lsr_end (line 198): `ND_LS_HDR(lr_b[0]) + lr_b[1] * sizeof sc_cur` becomes `ND_LS_HDR(lr_b[0] + lr_b[1])`.
4. nd_lsr_tri (line 208) becomes `    memcpy(&sc_cur, (const void*)(uintptr_t)lr_b[2u + n + ND_LS_SNAP(lr_b[2u + r])], sizeof sc_cur); /* NAT2D: the snap block of the range */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 2 Azahar starts: the bases are stored)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2d_host.txt 2>&1`: PASS. Commit `NAT2D 3: snap blocks, 256 unique snaps`.
2. `sh 3ds/tools/rowloop.sh --row NAT2D --bench fxparts_drec,spot_mac01_drec --no-host --why "NAT2D: snap blocks" > build3ds/out/nat2d_loop.txt 2>&1`.
3. `grep -H "c0src f\|lsover\|ndc: stream\|drec draw .* DIFF" build3ds/azt/logs/fxparts_drec.txt build3ds/azt/logs/spot_mac01_drec.txt build3ds/out/loop_NAT2D_cmp_*.txt > build3ds/out/nat2d_c0src.txt`.
4. `python 3ds/tools/c0gate.py build3ds/lane-NAT2D/pm_3ds.elf > build3ds/out/nat2d_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Build rc 0. Rowloop exit 0 (2 drec equal). c0gate `calls 126 interp 37 state 23`. No `lsover` line and no `ndc: stream` line in nat2d_c0src.txt.
Fix rights: compile errors. An `ndc: stream` line (the stream is full, nd_rec.c:47-52), a DIFF, lsover or a tool fault: stop, send nat2d_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT2D 4: gate. 2 drec equal; lsover 0` with `git add -f build3ds/out/nat2d_c0src.txt build3ds/out/nat2d_loop.txt`. It deletes this page. One SubagentHandback: hash, the 2 c0src lines.
