# Row HC4e4: delete the word fallbacks

(Model: Haiku. Start after HC4f3 is merged. Oracle batch 10, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e4`; PM_ROW=PM_LANE=HC4e4 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file and docs/handoff/row_HC4.md in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Goal
Every list is typed (gs and g forms: HC4e1-e3; map shapes: HC4f3). Delete the device code that types or reads raw F3DEX2 words; nd_op.c leaves the ELF (host tests only). Pictures byte-equal.

## Start state
Worktree build3ds/wt/HC4e4, branch HC4e4 from main at the HC4f3 merge. Copy assets/us and ver/us/build into it.

## Cause (oracle code read; line numbers at 384c404c2)
- Raw-word paths: nd_walk.c nd_walk_state :363-371, lstcen :214; nd_int.h nd_op_sync :159 (only user nd_walk.c :366); nd_rec.c sh_word :125 (only user :309), nd_rm_build :143-146, nd_rec_model :219-222, nd_rec_state :304-311, st_ok :323-329 (only user st_kind :334), nd_rec_list :436-457; nd_recs.c rec_op :34-39 (caller :91); src/model.c :2368 (nd_op_type_run); raw G_ENDDL sentinels nd_ent.c :206 and nd_dlc.c :491; nd_op.h nd_op_unit, nd_op_type_run, extern nd_op_n64; src/nd_op.c in 3ds/CMakeLists.txt :385.
- Each edit is an identity for typed input: nd_op_xlate passes a typed unit unchanged (nd_op.c :21); nd_op_type_run tests raw ids only, so a typed list passes whole.
- HC4e2a/HC4e2c notes: nd_ent_segments needs no edit: dlc_op (nd_dlc.c :12) maps NDO_END to OP_ENDDL and the loop breaks at :434 before ent_known. The nd_shape.c compile copy and dlc_hash read through nd_op_unit since HC4f3; nd_dlc_compile reads typed ids (dlc_op). Step B makes them typed-only.
- nd_dlc.c keeps dlc_op with its raw half: its switches use OP_ labels for typed ids, and nd_dlc_test feeds raw hand-made lists (pure module, host-tested).

## Step A: recorder side (no HC4f3 dependence; commit "HC4e4 recorder")
- A1 nd_rec.c nd_rm_build :143-146 become: `if ((w0 >> 24) == NDO_END) break;` / `if (IS_NOP(w0)) continue; /* a typed sync or no-op */` / `nd_rm_tab[i][2u * n] = w0; nd_rm_tab[i][2u * n + 1u] = w1; /* HC4e4: ModelRenderModes is typed (gs forms) */`.
- A2 nd_rec_model: delete :219 (R9/R10 xlate); :220 becomes `const unsigned n = words - G_PM_REC_WORDS; /* HC4e4: the R13 units are typed by the producer macros (nd_gbi.h) */`; delete the `wp -= ...` line :222. Fix the comment :209-210.
- A3 nd_rec_state :304-311 becomes `void nd_rec_state(uint32_t w0, uint32_t w1) { const uint32_t t[2] = { w0, w1 }; if (__builtin_expect(!nd_op_typed(w0 >> 24), 0)) { word_bad(w0); return; } rec_op_t(t); }`, with before it `static __attribute__((cold, noinline)) void word_bad(uint32_t w0) { static int k; if (k++ < 4) pm_log_e("ndc: untyped word %08x not recorded", (unsigned)w0); }`. Delete sh_word :125.
- A4 nd_rec_list: lines :436-437 become `const u32 u0 = g->words.w0, u1 = g->words.w1; u32 w0 = u0, w1 = u1; unsigned op = w0 >> 24;` / `if (!nd_op_typed(op)) { list_bad(g, n); return; } /* HC4e4: a static list is typed at compile time */`. Delete the `if (nd_op_typed(op)) {` wrapper line and its `}`; the four mapping lines stay. :454 and :457 call `nd_rec_state(u0, u1)`. Delete the `case G_PM_REC: case G_PM_SPR: case G_PM_ENT:` line :456.
- A5 st_kind :334 becomes `if (!nd_op_typed(op)) return 0u; /* HC4e4: no raw word in a typed list */`; delete st_ok :323-329; fix the comment :330-331.
- A6 nd_recs.c: delete rec_op and its comment :34-39; :91 becomes `rec_top((const uint32_t*)e); /* HC4e4: SolidCombineModes, AlphaTestCombineModes are typed (gs forms) */`.
- A7 src/model.c: delete the nd_op_type_run line :2368.
- A8 nd_ent.c :206 and nd_dlc.c :491 write `NDO_W0(NDO_END, 0)` in place of `G_ENDDL << 24` / `OP_ENDDL << 24`.
Gate A: nd_host PASS; lane build OK.

## Step B: readers and nd_op.c (needs HC4f3; commit "HC4e4 readers")
- B1 nd_walk.c nd_walk_state: delete `uint32_t t[2];` and the whole `if (!nd_op_typed(id)) { ... }` block; after the f3 line `if (q[0] == ND_NOP_A) continue;` add `if (__builtin_expect(!nd_op_typed(id), 0)) { op_unknown(dbg, q); continue; } /* HC4e4: every list is typed */`. Fix the comment :357-358. lstcen :214: drop the `!= G_ENDDL &&` half. nd_int.h: delete nd_op_sync :159.
- B2 every `nd_op_unit(A, B, T);` becomes `T[0] = A; T[1] = B;`: src/animator.c :912 :919 :966, src/effects/energy_orb_wave.c :259, src/entity_model.c :266 :307, src/imgfx.c :1319, src/model.c :4457 :4511 :4652, nd_ent.c :114 :134, nd_shape.c (dlc_hash, dlc_build, dlc_run_group, sbkw_dlchash), nd_seam.c list_decode, barricade.c, GetFirstTriangleNormal.inc.c, 3ds/tools/nd_seam_test.c tedges. nd_ent.c :107 ent_uid becomes `{ (void)w1; return w0 >> 24; }`.
- B3 nd_seam.c list_decode: delete the labels `case OP_NOOP: case OP_FULLSYNC: case OP_TILESYNC: case OP_PIPESYNC: case OP_LOADSYNC:` (NDO_GEOM stays). nd_host_test.sh line 27: remove `3ds/src/nd_op.c`.
- B4 nd_op.h: delete nd_op_unit and the nd_op_type_run declaration and comment; replace `extern const uint8_t nd_op_n64[...];` by the table from nd_op.c :7-12 as `static const uint8_t nd_op_n64[NDO_END_ - NDO_BASE] = {...};`; the nd_op_xlate comment says "host only (nd_op.c: nd_cmd_test, nd_model_test, nd_gv_test)". nd_op.c: delete the table and nd_op_type_run. 3ds/tools/nd_cmd_test.c: delete the HC4c bind list block that calls nd_op_type_run (:150-168; the closing brace of op_test at :169 stays). 3ds/CMakeLists.txt: delete `src/nd_op.c` :385.
Gate B: nd_host PASS; lane build OK (a link error names a missed user: fix it the same way).

## Done-check (paste the output in the step B commit message)
1. `grep -rn "nd_op_unit\|nd_op_type_run\|nd_op_xlate" src 3ds/src 3ds/include --include=*.c --include=*.h`: only 3ds/src/nd_op.c and the nd_op_xlate declaration and comments in 3ds/include/nd_op.h.
2. `grep -rnE "(>> ?(24|0x18)).{0,12}(G|OP)_(ENDDL|VTX|TRI1|TRI2|DL|GEOMETRYMODE|SPNOOP|RDP[A-Z]*SYNC|NOOP)\b|case (G|OP)_(ENDDL|VTX|TRI1|TRI2|DL)\b" src 3ds/src --include=*.c --include=*.h`: only src/animator.c :930 and src/model.c :4526 :4533 (the non-3DS `#else` branches), 3ds/src/nd_cen.c (census tables), 3ds/src/nd_dlc.c (dlc_op labels), 3ds/src/nd_op.c (host), 3ds/src/nd_rec.c nd_rec_list `case G_ENDDL` and `case G_DL` (typed units mapped). Any other hit: hand back.

## Done criteria (all must pass; handback only then)
1. nd_host PASS.
2. For spot_mac01_det, mdl5_iwa10_det, spot_mac00_det: `bench.py base <bench>`, `bench.py run <bench> --row HC4e4 --lane HC4e4 --why "..."`, `bench.py cmp <bench> --row HC4e4`: exit 0, 6/6 byte-equal. Run `bench.py base` only while no other Azahar run is on the machine until row TOOLB3 is merged. No `ndc: untyped word` line in the logs.
3. `PM_LANE=HC4e4 python 3ds/tools/bench.py run hotset_size --row HC4e4 --lane HC4e4 --why "..."`: rc 1 with m2_stale 16 = the 15 stale names of main plus rec_op (hot_order.ld :340, deleted in A6). Any other new stale name: hand back. Write m1_union in the commit. Row HOTORD regenerates hot_order.ld after this row.
Affected scenarios (no others): spot_mac01_det, mdl5_iwa10_det, spot_mac00_det.

## Commands
- Build: PM_ROW=HC4e4 PM_LANE=HC4e4 sh 3ds/build.sh > build.log 2>&1; then grep -n "error\|warning: unused" build.log.
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget HC4e4 (in the worktree).

## At the end
Delete this row file and docs/handoff/row_HC4.md in the completing commit. SendMessage main "HC4e4 ready <hash>".
