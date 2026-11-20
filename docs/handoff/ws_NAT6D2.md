# ws_NAT6D2 (Haiku: 1 file, core 0 only, byte-equal): nd_cpy_run runs its units without nr_set and ls_run
WAIT: `Merge NAT6D1` (real data dependency: this row copies the NAT6D1 rule into nd_cpy_run and removes the NAT6D1 compare).
Objective: nd_cpy_run calls a new cp_unit per copy state unit. cp_unit does what nr_set and ls_run do under NAT6D1 (the mark is the cut and the apply), with no nd_unit_same: the state notes of nd_unit_same move to cp_side. The c6 compare (c6_on, c6_note, nd_c6_bad) and the NAT6D1 branches in nr_set and ls_run go. Exact by construction. Counter C13: no change (FACT: nd_cpy_run loses ls_run and nr_set, cp_unit adds nd_tx_texon and nd_unit_apply). It prepares NAT6F.
Start commit: main after Merge NAT6D1. Worktree build3ds/wt/NAT6D2, branch NAT6D2. PM_ROW=NAT6D2 PM_LANE=NAT6D2. Designer: oracle NATIVE9 (id in the launch prompt). Regions (find by text): nd_walk.c the c6 statics line (after nd_epk_log), nr_set (the NAT6D1 lines), ls_run (the GEOM arm), nd_cpy_run and 2 new functions before it.
## Facts (NAT6B 9f79cae9a6 plus ws_NAT6D1)
- FACT nd_unit_same changes state only for OMH (om_kh |= m when the bits are equal and sc_ok), OML (om_kl |= m when equal) and PRIM (pm_gpu_set_prim and prim_lod when equal, same LOD byte and sc_ok) (nd_set.c:126-134). GEOM, ENV, FOG, BLEND, TEXTURE, COMB and MW are pure reads (pm_gpu_mode_is pm_gpu.c:3719, sc_sig_now nd_comb.c:158).
- FACT under NAT6D1: in mode 2, ls_run skips OML, PRIM..BLEND, RARE, COMB before any nd_unit_same, runs OMH (ls_omh) and TEXTURE (nd_tx_texon) always, and GEOM applies on the mark with no cut. Every other unit goes to nr_set: nd_unit_same first, then at mode 0 and ex_depth 0 the combine class (OML, PRIM..PRIMZ, RARE, COMB) returns, else on the mark: pm_gbi_cur_op = id, gv_epoch++, the debug hook, nd_unit_apply.
- FACT in mode 2 ex_depth is above 0 or ep_stream is 0, so the stream test of nr_set is false there. In mode 0 and 1 nd_cpy_run sets ep_stream = (mode == 0).
## Edits (each new or changed line ends with `/* NAT6D2 */`)
1. nd_walk.c, before `void nd_cpy_run(`:
```c
static void cp_side(const uint32_t* u) { const uint32_t a = u[0] & 0xffffffu, b = u[1]; const unsigned id = u[0] >> 24; /* NAT6D2: the state notes of nd_unit_same (nd_set.c:126-134) */
    if (id == ND_FS_OMH) { const uint32_t m = nd_op_om_mask(a); if ((b & m) == (nd_ws.omode_h & m) && sc_ok) nd_ws.om_kh |= m; }
    else if (id == ND_FS_OML) { const uint32_t m = nd_op_om_mask(a); if ((b & m) == (nd_ws.omode_l & m)) nd_ws.om_kl |= m; }
    else if (id == ND_FS_PRIM && nd_ws.prim == b && (uint8_t)nd_ws.prim_lod == (uint8_t)a && sc_ok) { pm_gpu_set_prim(nd_ws.prim, a); nd_ws.prim_lod = a; } }
static void cp_unit(unsigned dbg, const uint32_t* u, unsigned m, int mk) { const unsigned id = u[0] >> 24; int cut = 1; /* NAT6D2: one copy state unit, nr_set and ls_run under NAT6D1 */
    if (m == 2u) { if (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB) return;
        if (id == ND_FS_OMH) { ls_omh(u); return; } if (id == ND_FS_TEXTURE) { nd_tx_texon(u[0] & 0xffffffu, u[1]); return; } if (id == ND_FS_GEOM) cut = 0; else cp_side(u); }
    else { cp_side(u); if (m == 0u && ex_depth == 0 && (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_PRIMZ) || id == ND_FS_RARE || id == ND_FS_COMB)) return; }
    if (!mk) return;
    int fp = 0; if (cut) { pm_gbi_cur_op = id; gv_epoch++; if (__builtin_expect(dbg, 0)) fp = walk_dbg_op(dbg, (const NdW2*)u, 0, id, u[0], u[1]); }
    nd_unit_apply(u); if (__builtin_expect(fp, 0)) pm_ph_pop(); }
```
2. nd_cpy_run loop. Before: `        c6_on = 1; if (m == 2u) ls_run(q, 1u); else nr_set(dbg, q, q); c6_on = 0; c6_mark = 0; }`. After: `        cp_unit(dbg, q, m, c6_mark != 0); c6_mark = 0; } /* NAT6D2 */`.
3. nr_set: delete the 2 NAT6D1 lines `if (c6_on) { ... if (s0 && ca_) return; ...` and `if ((s0 ? 0 : !same) != mk ... } else ...`, so the line `if (ep_stream && ex_depth == 0 && (ca_ || cb_)) {` follows the ca_ line again. Keep `int cut = !same, ap = !same;`, `if (!ap && !cut) return;` and `if (ap) nd_unit_apply(u);`.
4. ls_run, the GEOM arm. Before (NAT6D1): `else if (id == ND_FS_GEOM) { const int sm = nd_unit_same(q), ap = c6_on ? ... } /* NAT6D1 */`. After: `else if (id == ND_FS_GEOM) { if (!nd_unit_same(q)) nd_unit_apply(q); } /* NAT6D2 */`.
5. The c6 statics line. Before: `static unsigned char c6_mark, c6_on; unsigned nd_c6_bad; static ... void c6_note(void) { ... } /* NAT6B */`. After: `static unsigned char c6_mark; /* NAT6B: the slot of the copy unit that runs; NAT6D2 */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only, 1 build, 8 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat6d2_host.txt 2>&1`: PASS. Commit `NAT6D2 1: nd_cpy_run runs its units without nr_set and ls_run`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec > build3ds/out/nat6d2_base1.txt 2>&1`, the same for fxparts_drec (nat6d2_base2.txt). 3. `sh 3ds/tools/rowloop.sh --row NAT6D2 --bench entcape_kmr03_drec,fxparts_drec --no-host --why "NAT6D2: cp_unit" > build3ds/out/nat6d2_loop.txt 2>&1`.
4. `mkdir -p build3ds/out/c0base && git archive $(git merge-base HEAD main) | tar -x -C build3ds/out/c0base`, then `python build3ds/out/c0base/3ds/tools/c0gate.py build3ds/lane-NAT6D2/pm_3ds.elf` and `python 3ds/tools/c0gate.py build3ds/lane-NAT6D2/pm_3ds.elf`, both into build3ds/out/nat6d2_c0.txt.
## Gate (exact): host PASS; LOOP PASS (2 benches byte-equal against the NAT6D1 merge); the 2 c0gate lines are equal. Fix rights: compile errors, unused warnings, prototypes. A DIFF, a crash, other c0gate values or a tool fault: stop. Send the cmp path and its first DIFF line to oracle NATIVE9 with SendMessage. Wait.
## Hand-back: last commit `NAT6D2 2: gate. <lines>` with `git add -f build3ds/out/nat6d2_loop.txt build3ds/out/nat6d2_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, c0gate lines.
