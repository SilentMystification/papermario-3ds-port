# ws_VSLOT2: triangles over 2 vertex arrays and triangles of caller slots get v2 draws; the effect lists that become ok move (C5, C6, C7)
Model: Sonnet (why not Haiku: 5 coupled C edits in the resolver, the emitter and the host model; a wrong index base shows only as a host compare fail).
PM_ROW, PM_LANE = VSLOT2. Branch VSLOT2, worktree build3ds/wt/VSLOT2. Budget: 1 build, 8 Azahar starts (base runs not charged). Start now. Parallel with ws_SBJOIN.
Owns: 3ds/tools/nd_vslot.h, nd_vslot.c, nd_vslot_test.c, nd_gfx_conv.c, nd_gtab_test.c, gset.py, 3ds/include/nd_gtab.h (:101 comment only), 3ds/tests/host.d/gset.sh, 3ds/tests/gset/case_inc/ (new), 3ds/tools/gset.d/VSLOT2.txt (new), and the files gset.py writes (src/effects, src/entity, 3ds/tools/gfxin). Edit no other file.
FACT (oracle nd_host, main 31a2c110b2): `gconv ntab tables 1488 inherit 72 unsup 27`, `gconv gset ok 1396 of 1546`, `moved 1319`; 66 lists fail `vertex slots` (ntab_unsup.txt 16, `itab unsup` 50).
FACT (nd_vslot.c:30-35): a triangle fails when a slot is not loaded (12 callees that use the caller's slots, e.g. D_090023E0_37A360) or when its slots hold 2 arrays (e.g. D_09002CE8_3C0688). FACT: the device reads only the units (ndnu, nd_ntab_run); NDN_DRAW is read only by the host compare (nd_gtab_test.c:439). Group 1 changes no device byte.
## Group 1: resolver, converter, host model (0 builds, 0 runs, commit `VSLOT2 1: ...`)
1. nd_vslot.h after the enum: `#define ND_VS_MIX 0xFFFFFFFFu /* VSLOT2: a draw over 2 or more arrays: idx indexes ref */`, `#define ND_VS_IN 0xFFFFFFFEu /* VSLOT2: idx = caller slots */`,
   `typedef struct NdVsRef { uint32_t arr, index; } NdVsRef;`. Append `, NdVsRef* ref, unsigned ref_max, unsigned* nref` to the nd_vslot_resolve arguments (.h and .c:9-10).
2. nd_vslot.c :14 add `unsigned r, nr = 0;`. Replace :30-39 (tarr and the j loop): nl = loaded slots of the 3 (s[j] >= VS_SLOTS fails as now); nl 1 or 2: `return -(int)(i + 1);`;
   nl 0: `tarr = ND_VS_IN; tix[j] = s[j];`; nl 3: `tarr = slot[s[0]].arr`, ND_VS_MIX when a slot arr differs; one arr: `tix[j] = slot[s[j]].index;`; MIX: r = the entry of (slot arr, slot index)
   in ref[0..nr), else append it (nr >= ref_max fails), `tix[j] = r;`. The draw grouping (:40-50) stays. Before :54 add `*nref = nr;`.
3. nd_vslot_test.c: `static NdVsRef g_ref[16]; static unsigned g_nref;`; :62 adds `g_ref, 16, &g_nref`. E1 (:119): rc 0, draw {ND_VS_IN, 0, 3}, idx {5, 6, 7}. E2 (:125): rc 0, draw {ND_VS_MIX, 0, 3},
   idx {0, 1, 2}, g_nref 3, g_ref {1,0},{1,1},{2,0}. New E4: `vtx(1, 0, 1, 0), tri1(0, 5, 6)` rc -2. `pass != 7`, print `vslot ok 7/7` (and the :1 comment).
4. nd_gfx_conv.c V2C after :211: `NdVsRef ref[V2_MAXIDX]; unsigned nref; /* VSLOT2: refs of the mixed draws (ndnv_<nm>) */`. v2_flush (:302-322): static `NdVsRef ref[V2_MAXIDX]`, `nref` to resolve;
   after the `if (r)` line: `if (cx->nref + nref > V2_MAXIDX) { v2_unsup("too many vertex refs", 0); return; }` `const unsigned rb = cx->nref; memcpy(&cx->ref[rb], ref, nref * sizeof ref[0]); cx->nref += nref;`.
   Draw loop: `kind = MIX ? 1 : IN ? 2 : 0`; p = `(const void*)ndnv_<cx->nm>` (1), `0` (2), the old text (0).
   Before `v2_push(NDN_DRAW, 0, dr[i].nidx, 0, p, cx->nidx); memcpy(&cx->idx[cx->nidx], &idx[dr[i].idx0], ...);`
   After `v2_push(NDN_DRAW, kind, dr[i].nidx, 0, p, cx->nidx); for (q ...) cx->idx[cx->nidx + q] = (uint16_t)(idx[dr[i].idx0 + q] + (kind == 1u ? rb : 0u));`. 5. v2_emit_table after :616, `if (c->nref)`: print `static const void* const ndnv_%s[%u] = {`, per ref `v2_sym_expr(e, sizeof e, (uintptr_t)gconv_sym[c->ref[i].arr].addr + 16u * c->ref[i].index, GS_VTX)`
   (0: print `gconv: %s: vertex ref outside the set`, exit 1), then `};`. It stays inside the `#ifdef ND_HOST_GFX` of a moved list (:603-617). nd_gtab.h:101 comment: a 1 = ndnv pointers, a 2 = caller slots.
6. nd_gtab_test.c:222 `const void* base;` -> `const void* base[3];`. m_tri (:367-377): nl = ok slots; nl 1 or 2: `m_over = 1; return;`; `base[j] = nl ? mslot[s[j]].base : NULL; i[j] = nl ? mslot[s[j]].idx : s[j];`.
   v_replay (:444-445), x = `t->idx[o->idx_pos + j + q]`: a 1: vp = `((const void* const*)o->p)[x]`, vs = `m_vsym((uint32_t)(uintptr_t)vp)`, base vs->addr, i (vp - addr) / 16 (vs NULL: NULL, ~0u);
   a 2: base NULL, i x; a 0: base o->p, i x. :625 compares base[0..2] and i[0..2].
7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/vslot2_h1.txt 2>&1`: `ND HOST TESTS PASS`, `vslot ok 7/7`, `bad 0` on the 2nd and 3rd `ntab tables` lines (the 1st is the seeded fault),
   `unsup` below 27, `gconv gset ok` above 1396 (GUESS: unsup about 11, gset ok +40..+66). Commit with the gconv lines in the message.
## Group 2: gset include hold (0 builds, 0 runs, commit `VSLOT2 2: ...`)
8. gset.py after :23 `INC_HOLD = True  # VSLOT2: an include whose list a kept line of the device file names stays; seeded test in gset.sh`. move_includes after :69 `rest = "\n".join(x for x in lines if not INC.match(x))`.
   :82 Before `if names and all(n in ok for n in names):` After `if names and all(n in ok for n in names) and not (INC_HOLD and any(re.search(r"(?<!\w)" + re.escape(n) + r"(?!\w)", rest) for n in names)):`.
9. Fixture 3ds/tests/gset/case_inc: list.txt `src/effects/gfx/w.c`; gset_ok.txt `C` `D` `W`; assets/us/effects/gfx/C.gfx.inc.c `Gfx C[] = {` `    gsSPEndDisplayList(),` `};` (D the same); w.c: the 2 includes,
   `Gfx W[] = {` `    gsSPBranchList(C),` `};`. gset.sh before :63: rc 0, a `gset moved 1 files` line, w.c keeps the C include; seeded: `INC_HOLD = False` moves it (the grep fails). Host PASS. Commit.
## Group 3: the move (0 builds, 0 runs, commit `VSLOT2 3: ...`)
10. `python 3ds/tools/listcen.py > build3ds/out/vslot2_lc0.txt`; `cut -d' ' -f1,2,4- build3ds/out/ntab_entries.txt > build3ds/out/vslot2_e0.txt`. `{ ls src/effects/gfx/*.c; echo src/entity/model/CymbalPlant_gfx.c; }
    > 3ds/tools/gset.d/VSLOT2.txt` (LF). `python 3ds/tools/gset.py 3ds/tools/gset.d/VSLOT2.txt build3ds/out/gset_ok.txt > build3ds/out/vslot2_set.txt 2>&1`: `gset ok`. Example (snowfall.c): the D_09000D30 and
    D_09000D50 includes move, the D_09000C10 include stays (step 8). Example (user): `nd_draw_list(D_09002CE8_3C0688, ...)` -> `nd_draw_list(NDN(D_09002CE8_3C0688), ...)`.
11. Host into vslot2_h3.txt: PASS, `bad 0` on `gconv lists`, no `gconv: moved` line. The same cut into vslot2_e1.txt; `cmp` of e0 and e1 prints nothing. Commit.
## Group 4: the gate (1 build, 5 charged runs, commit `VSLOT2 4: ...`)
12. One build: rc 0, memcheck margin above 0 KB (FACT main: 442 KB). For B in spot_mac01_drec sbk14_drec fxr_drec fxp_drec fxpic_20_psochk: `bench.py base $B --row VSLOT2 --commit $(git merge-base HEAD main)`,
    `bench.py run $B --row VSLOT2 --why "VSLOT2 4"`, `bench.py cmp $B --row VSLOT2 --commit $(git merge-base HEAD main)`. Pass: drec byte-equal; the `psochk end ... bad N` value equals the base run (B1).
13. `python 3ds/tools/listcen.py > build3ds/out/vslot2_lc1.txt` (GUESS: C5 -900..-1300, C6 -40..-70, C7 -40..-70; basis: listcen data of the 66 lists' files). Delete this page and its INDEX.md line. Commit. Fix rights: a compile error in your own edit lines. Step 10 or 11 not as given: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`. A host compare fail, a cmp difference or a failed base
run: write the output to build3ds/out/, send the path to the oracle (CLAUDE.md 4 and 5). Never change a base. Stop: group 4 committed and gated. Hand-back build3ds/out/handback_VSLOT2.txt, first 8 lines: commit hashes, gconv ntab and gset ok lines (before, after), `gset moved` line, vslot line, the 5 cmp results, C5 C6 C7 (lc0, lc1), margin, Azahar starts.
