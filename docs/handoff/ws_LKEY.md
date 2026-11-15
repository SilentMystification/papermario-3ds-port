# ws_LKEY (Haiku): list readers read the device units of a moved list key (the VCOUNT class), and a reader census host test
Objective: FACT (code read): NDN(x) (nd_gtab.h:73) and the v1 table of a moved list (nd_gfx_conv.c:173, NDG_F_MOVED) point to an NdNDev (20 bytes),
not to list units. These readers walk such a pointer as list words: entity_model.c:281 ent_vtx_range and :322/:337 entcheck_draw (debug switch
entcheck: 51 STANDARD_ENTITY_MODEL_SCRIPT(NDN(..)) keys, e.g. coconut.c:30), nd_ent.c:526 (entcheck end record), energy_orb_wave.c:258-267 (debug
switch orbdbg), nd_ent.c:233 ent_tdwalk, :113 ent_hash_walk, :140-141 ent_flat (release path, read only), nd_walk.c:544 gt_call (release, depth limit),
nd_rec.c:148-150 nd_rm_build (ModelRenderModes, model.c:182: a key when the Gfx_RM lists move). Fix: one shared helper (nd_lkey.h). Then a census
host test fails on any new list word reader. Counter: none (a fault class). Report listcen C7 and C8 before and after (GUESS: C8 +1 to +2).
Worktree build3ds/wt/LKEY, branch LKEY. Edit only the files of ws_LKEY_code.md (blocks A to J). Do not edit gset.py, nd_gfx_conv.c, model.c.
## Edit shapes (exact text: ws_LKEY_code.md)
- A walk of `l[k].words` becomes a walk of `u[2u * k]`, u from nd_lkey_units. Example, nd_ent.c:113-114. Before:
  `for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {` / `u32 w0 = l[i].words.w0, w1 = l[i].words.w1;`
  After: `const uint32_t* lu = nd_lkey_units(l, nd_ntab_find(l), ND_DLC_MAXOPS, &nl);` / `for (unsigned i = 0; i < nl; i++) {` / `u32 w0 = lu[2u * i], w1 = lu[2u * i + 1u];`
- A reader that needs the end unit gets nd_lkey_end (a copy with an NDO_END unit). Example, entity_model.c:337. Before:
  `nd_dlc_tex_decode((const uint32_t*)(const void*)q, &tdec)`. After: `nd_dlc_tex_decode(qu, &tdec)` (qu from nd_lkey_end, block D3).
- A v1 table walk uses nd_lkey_tab. Example, nd_walk.c:544. Before: `nd_walk_state((const uint32_t*)c->list, ND_LIST_MAX);`
  After: `{ unsigned nc; const uint32_t* cu = nd_lkey_tab(c, ND_LIST_MAX, &nc); nd_walk_state(cu, nc); }`
## Loop (1 build, 2 Azahar runs; base runs not charged)
1. Group 1: blocks A, B, C (helper and host test). `sh 3ds/tools/nd_host_test.sh > build3ds/out/lkey_host1.txt 2>&1`.
   Pass: ND HOST TESTS PASS and `lkey ok`. The seed in lkey.sh must print `lkey FAIL case 0`. Commit `LKEY 1`.
2. Group 2: blocks D to H (the readers). Host test again (> build3ds/out/lkey_host2.txt): PASS. Commit `LKEY 2`.
3. Group 3: blocks I and J (census). Run `python 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .` and fill the 4 `<n>` counts (block J1 rule).
   Host test (> build3ds/out/lkey_host3.txt). Pass: PASS, `lkey census test ok`, the census line `lkey census ok files 22 lines <m>`
   (GUESS 22: FACT 23 files on main 4a6d25d191, energy_orb_wave.c leaves in group 2). Commit `LKEY 3`.
4. One build. memcheck margin above 0 (434 KB now; GUESS -4 KB: qb and eb, 2 KB each).
5. Gate: `bench.py base <b>`, `bench.py run <b> --row LKEY --why "LKEY gate"` (PM_ROW=LKEY, PM_LANE=LKEY), then
   `bench.py cmp <b> --row LKEY --commit $(git merge-base HEAD main)` for spot_mac01_drec and sbk14_drec. Pass: 1/1 byte-equal each.
## Tests that decide
Host: lkey.sh (7 cases, seeded fault), lcen.sh (census of the tree, seeded fault case_new). Device: 2 drec byte compares.
## Fix scope
A failing host test: fix only nd_lkey.h, nd_lkey_test.c, lkey.sh, lkey_cen.py, lcen.sh or the 4 counts in allow.txt. A compile error in a reader
edit: fix the edit only (type or name), keep its logic. A differing drec, a census path not in block J1, or a count that is not a reader of
group 2: stop, write the output to build3ds/out/, send the path to the oracle (SendMessage). Two tries per named cause.
## Stop and hand-back
Stop: all gates pass. Commit `LKEY 4: gate` with the values. Delete this page and ws_LKEY_code.md in that commit.
Hand-back: build3ds/out/handback_LKEY.txt (commit, host lines lkey ok and census, memcheck margin, 2 cmp results, listcen C7 C8 before and after).
