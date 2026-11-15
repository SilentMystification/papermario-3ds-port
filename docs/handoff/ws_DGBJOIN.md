# ws_DGBJOIN: dgb_01 gfx.c joins the converter set; its vertex loads stay inside their arrays; its ok lists move to the gfxin twin (C5, C6, C7)
Model: Haiku. PM_ROW, PM_LANE = DGBJOIN. Branch DGBJOIN, worktree build3ds/wt/DGBJOIN. Budget: 1 build, 8 Azahar starts (base runs not charged). Start now.
Parallel with ws_MESH1 (it edits other lines of nd_gfx_conv.c). Owns: 3ds/tools/nd_gfx_conv.c line 592 only, vtxsplit.py, gconv_src.d/DGBJOIN.txt, gset.d/DGBJOIN.txt (new), src/world/area_dgb/dgb_01/gfx.c,
src/world/area_dgb/dgb_01/smash_bridges_skeleton.inc.c, the twin 3ds/tools/gfxin/src/world/area_dgb/dgb_01/gfx.c. Edit no other file.
FACT (listcen, main f495844bdf): dgb_01/gfx.c has C5 data 2833 of 8914 and C6 arrays 168 of 651. Its lists draw only in the Tubba bridge scene (bridges.c:10, animator model nodes).
FACT (oracle dry run, scratch worktree): with the 10 sources of step 3 joined, gconv lists 1570 -> 1746, unsup 19 -> 44: 25 dgb_01 lists `vertex ref outside the set`, gset ok +0.
FACT cause: 69 loads in 21 lists read past their array end into the next array in ROM order. gfx.c:14 `gsSPVertex(vtx_C2E620, 32, 0)`, but vtx_C2E620 has 25
(assets/us/world/area_dgb/vtx/C2E620.vtx.inc.c:3). The device map has the arrays in a row (build3ds/pm_3ds.map: vtx_C2E620 0x00bde908, vtx_C2E7B0 0x00bdea98), the host does not.
The split makes each load stay inside one array: the same vertices go to the same slots on the device, without a link order rule. FACT (dry run after the split):
unsup 1 dgb_01 list, `call target outside the set or too deep` (D_80251A50_C0F2D0 calls a chain of 10 lists, gfx.c:3337), and gset ok +0 (that list blocks the shared img callees).
FACT: the converter stops at depth 8 (nd_gfx_conv.c:592); the device runs 12 levels (ND_LIST_DEPTH, nd_walk.c:181). FACT: no main list fails by depth (ntab_unsup.txt).
## Group 1: vtxsplit (0 builds, commit `DGBJOIN 1: ...`)
1. New 3ds/tools/vtxsplit.py `<vtx dir> <gfx file>`: read each `Vtx NAME[N]` of `<vtx dir>/*.vtx.inc.c` (size N). For each line `<ind>gsSPVertex(NAME, n, v),` or
   `<ind>gsSPVertex(&NAME[k], n, v),` with k + n > N: write loads of c = min(n, N - k) from `NAME` (k 0) or `&NAME[k]` to slot v, then n -= c, v += c, and go on at k 0 of the
   next array in ROM order: `vtx_%X` of (int(NAME[4:], 16) + N * 16). Exit 1 when that array does not exist. Other lines stay. Print `vtxsplit lines <split lines> loads <new loads>`.
2. `python 3ds/tools/vtxsplit.py assets/us/world/area_dgb/vtx src/world/area_dgb/dgb_01/gfx.c`: prints `vtxsplit lines 69 loads 173` (FACT, oracle dry run). A second run prints
   `vtxsplit lines 0 loads 0`. Example (gfx.c:14). Before `    gsSPVertex(vtx_C2E620, 32, 0),` After `    gsSPVertex(vtx_C2E620, 25, 0),` `    gsSPVertex(vtx_C2E7B0, 5, 25),`
   `    gsSPVertex(vtx_C2E800, 2, 30),`. nd_gfx_conv.c:592 Before `depth >= 8u` After `depth >= 11u` (a comment: the device list depth 12, nd_walk.c:181).
   Host tests (nd_host_test.sh into dgbjoin_h0.txt): PASS, the gconv and ntab lines equal to main (lists 1570, unsup 19, gset ok 1461). Commit.
## Group 2: join (0 builds, commit `DGBJOIN 2: ...`)
3. New 3ds/tools/gconv_src.d/DGBJOIN.txt (LF), 10 lines: `src/world/area_dgb/dgb_01/gfx.c`, `src/world/area_dgb/dgb_01/img0.c` .. `img7.c`, `src/world/area_dgb/smash_bridges.c`.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dgbjoin_h1.txt 2>&1`: `ND HOST TESTS PASS`, `bad 0` on `gconv lists` (1746 lists) and on the 2nd and 3rd `ntab tables` lines.
   FACT (dry run with steps 1-3): unsup 19, gset ok 1461 -> 1637 (all 176 dgb_01 lists), ntab tables 1722. `grep -c dgb_01 build3ds/out/gset_ok.txt` goes in the commit message. Commit.
## Group 3: the move (0 builds, commit `DGBJOIN 3: ...`)
5. `python 3ds/tools/listcen.py > build3ds/out/dgbjoin_lc0.txt`. `cut -d' ' -f1,2,4- build3ds/out/ntab_entries.txt > build3ds/out/dgbjoin_e0.txt`.
6. New 3ds/tools/gset.d/DGBJOIN.txt: one line `src/world/area_dgb/dgb_01/gfx.c`. `python 3ds/tools/gset.py 3ds/tools/gset.d/DGBJOIN.txt build3ds/out/gset_ok.txt > build3ds/out/dgbjoin_set.txt 2>&1`:
   last line `gset ok`. Block mode moves each ok `Gfx N(x)[] = {` block (gset.py:27 BLOCK_OPEN). The img lists sit in assets includes and stay.
   Example (gfx.c:6). Before `Gfx N(D_8024B770_C08FF0)[] = {` .. `};` After one line `NDN_DECL(N(D_8024B770_C08FF0));`; the block goes to the twin.
   Example (smash_bridges_skeleton.inc.c:6). Before `.displayList = N(D_8024C098_C09918),` After `.displayList = NDN(N(D_8024C098_C09918)),`.
   Precedent: src/battle/move/item/coconut.c:28-30. Precedent of a moved list on an animator model node: MunchlesiaChewing_anim.c:8 (animator.c:898 draws it).
7. Host into dgbjoin_h2.txt: PASS, `bad 0` on `gconv lists`, no `gconv: moved` line. The same cut into dgbjoin_e1.txt; `cmp build3ds/out/dgbjoin_e0.txt build3ds/out/dgbjoin_e1.txt` prints nothing. Commit.
## Group 4: the gate (1 build, 4 charged runs, commit `DGBJOIN 4: ...`)
8. One build: rc 0, memcheck margin above 0 KB (FACT main: 454 KB). For B in spot_mac01_drec sbk14_drec fxr_drec fxp_drec: `bench.py base $B --row DGBJOIN --commit $(git merge-base HEAD main)`,
   `bench.py run $B --row DGBJOIN --why "DGBJOIN 4"`, `bench.py cmp $B --row DGBJOIN --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
   FACT: no bench draws the bridge scene. The host unit stream compare (step 7) is the main proof of the moved lists.
9. `python 3ds/tools/listcen.py > build3ds/out/dgbjoin_lc1.txt` (GUESS C5 -2833, C6 -168: all 168 gfx.c blocks are ok). Delete this page and its INDEX.md line. Commit.
Fix rights: a compile error of a rewritten line in your own files. A `gset keep` line: edit that line by hand to the precedent form, note it in the commit.
Step 6 or 7 not as given: `git checkout -- src 3ds/tools/gfxin`, `git clean -fdq -- 3ds/tools/gfxin`. A cmp difference or a failed base run: write the output to build3ds/out/,
send the path to the oracle (CLAUDE.md 4 and 5). Never change a base. Stop: group 4 committed and gated. Hand-back build3ds/out/handback_DGBJOIN.txt, first 8 lines:
commit hashes, the vtxsplit line, gconv lists and gset ok (h1, h2), the `gset moved` line, the 4 cmp results, C5 C6 C7 (lc0, lc1), memcheck margin, Azahar starts used.
