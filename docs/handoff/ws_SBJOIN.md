# ws_SBJOIN: SaveBlock.c, UltraBlock.c and UltraBlockContent.c join the converter set; their ok lists move to the gfxin twins (C5, C6, C7)
Model: Haiku. PM_ROW, PM_LANE = SBJOIN. Branch SBJOIN, worktree build3ds/wt/SBJOIN. Budget: 1 build, 8 Azahar starts (base runs not charged). Start now. Parallel with ws_VSLOT2.
Owns: 3ds/tools/gconv_src.d/SBJOIN.txt (new), 3ds/tools/gset.d/SBJOIN.txt (new), src/entity/model/SaveBlock.c, UltraBlock.c, UltraBlockContent.c, src/entity/SaveBlock.c,
src/entity/SuperBlock.c, and the twins gset.py writes in 3ds/tools/gfxin/src/entity/model. Edit no other file.
FACT (listcen, main 31a2c110b2): C5 data SaveBlock.c 140, UltraBlock.c 84, UltraBlockContent.c 34. FACT: none of the 3 is in 3ds/tools/gconv_src.d (S7ENT.txt has 49 other entity models).
FACT (gen_gfx_syms.py:15): the scan drops `#if VERSION_PAL` .. `#endif` blocks, so the PAL lists of SaveBlock.c (:53-65, :101-128, :195-201) are not in the set and stay.
FACT (precedents on main): ENTITY_ADDR(entity, Gfx*, NDN(x)) at src/entity/Chest.c:106; STANDARD_ENTITY_MODEL_SCRIPT(NDN(x), ...) at src/battle/move/item/dusty_hammer.c:29;
`data->gfx1 = NDN(Entity_SuperBlockContent_Render);` at src/entity/SuperBlock.c:212 with the draw `gSPDisplayList(gMainGfxPos++, dlist);` at :150.
## Group 1: join (0 builds, 0 runs, commit `SBJOIN 1: ...`)
1. New 3ds/tools/gconv_src.d/SBJOIN.txt (LF), 3 lines: `src/entity/model/SaveBlock.c`, `src/entity/model/UltraBlock.c`, `src/entity/model/UltraBlockContent.c`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/sbjoin_h1.txt 2>&1`: `ND HOST TESTS PASS`, `bad 0` on the `gconv lists` line (lists above 1546), `bad 0` on the 2nd and 3rd `ntab tables` lines.
3. `grep -E "^Entity_(SaveBlock|UltraBlock)" build3ds/out/gset_ok.txt > build3ds/out/sbjoin_ok.txt` and `grep -hE "^Entity_(SaveBlock|UltraBlock)" build3ds/out/ntab_unsup.txt build3ds/out/ntab_inherit.txt > build3ds/out/sbjoin_no.txt`.
   GUESS: most of the 24 US lists are ok (IENTRY gives an inherit list a table). Put the line count of sbjoin_ok.txt in the commit message. Commit.
## Group 2: the move (0 builds, 0 runs, commit `SBJOIN 2: ...`)
4. `python 3ds/tools/listcen.py > build3ds/out/sbjoin_lc0.txt`. `cut -d' ' -f1,2,4- build3ds/out/ntab_entries.txt > build3ds/out/sbjoin_e0.txt`.
5. New 3ds/tools/gset.d/SBJOIN.txt: the same 3 lines as step 1. `python 3ds/tools/gset.py 3ds/tools/gset.d/SBJOIN.txt build3ds/out/gset_ok.txt > build3ds/out/sbjoin_set.txt 2>&1`: last line `gset ok`.
   Block mode moves each ok `Gfx X[] = {` block. A block that an unmoved block names stays (fixpoint, gset.py:174-183). GUESS: Entity_SaveBlock_RenderFaces stays (the PAL block RenderBlock_es names it).
   Example (src/entity/model/UltraBlock.c:121). Before `Gfx Entity_UltraBlock_Render[] = {` .. `};` After one line `NDN_DECL(Entity_UltraBlock_Render);`; the block goes to 3ds/tools/gfxin/src/entity/model/UltraBlock.c.
   Example (src/entity/SuperBlock.c:220). Before `data->gfx1 = Entity_UltraBlockContent_Render;` After `data->gfx1 = NDN(Entity_UltraBlockContent_Render);`.
   Example (src/entity/SaveBlock.c:51). Before `dlist = ENTITY_ADDR(entity, Gfx*, Entity_SaveBlock_RenderBlock);` After `dlist = ENTITY_ADDR(entity, Gfx*, NDN(Entity_SaveBlock_RenderBlock));`.
6. Host into sbjoin_h2.txt: `ND HOST TESTS PASS`, `bad 0` on `gconv lists`, no `gconv: moved` line. The same cut into sbjoin_e1.txt; `cmp build3ds/out/sbjoin_e0.txt build3ds/out/sbjoin_e1.txt` prints nothing. Commit.
## Group 3: the gate (1 build, 4 charged runs, commit `SBJOIN 3: ...`)
7. One build: rc 0, memcheck margin above 0 KB (FACT main: 442 KB). For B in spot_mac01_drec sbk14_drec fxr_drec fxp_drec: `bench.py base $B --row SBJOIN --commit $(git merge-base HEAD main)`,
   `bench.py run $B --row SBJOIN --why "SBJOIN 3"`, `bench.py cmp $B --row SBJOIN --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
   FACT: mac_01 has a save point (src/world/area_mac/mac_01/entity.c names Entity_SavePoint). OPEN: whether frames 200 and 201 of spot_mac01_drec draw it. The host unit stream compare is the main proof.
8. `python 3ds/tools/listcen.py > build3ds/out/sbjoin_lc1.txt` (GUESS: C5 -150..-250, C6 -20..-26, C7 -20..-30). Delete this page and its INDEX.md line. Commit.
Fix rights: a compile error of a rewritten line in your own files. A `gset keep` line: edit that line by hand to the precedent form above, note it in the commit.
Step 2 fails the host compile of a joined source: delete the line of that source from SBJOIN.txt (step 1 and step 5), note it in the commit, go on with the other sources.
Step 5 or 6 not as given: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`. A cmp difference or a base run that fails: write the output to build3ds/out/,
send the path to the oracle (CLAUDE.md 4 and 5). Never change a base.
Stop: group 3 committed and gated. Hand-back build3ds/out/handback_SBJOIN.txt, first 8 lines: commit hashes, the gconv lists and gset ok lines (before, after), the sbjoin_ok.txt count,
the `gset moved` line, the 4 cmp results, C5 C6 C7 (lc0, lc1), memcheck margin, Azahar starts used.
