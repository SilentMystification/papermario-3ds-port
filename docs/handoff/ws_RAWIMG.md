# ws_RAWIMG (Haiku): Heart and Ultra block content lists move to device keys (C5 -69, C6 -12, C7 -17)
Designer: the oracle (SendMessage). Worktree build3ds/wt/RAWIMG, PM_ROW=RAWIMG PM_LANE=RAWIMG. Main at release: c687bd4660. FACT: `git apply --check` OK on main and on main + FXOPS + FXLT + ENTCAPE patches.
Start: after `Merge ENTCAPE` (ENTCAPE edits HeartBlock.c and SuperBlock.c and adds the bench entcape_kmr03_drec).
Files (all in docs/handoff/ws_RAWIMG.patch, 9 files): 3ds/tools/gen_gfx_syms.py, new 3ds/tests/host.d/rawimg.sh,
new 3ds/tools/gset.d/RAWIMG.txt, src/entity/model/HeartBlockContent.c, src/entity/model/UltraBlockContent.c,
new 3ds/tools/gfxin/src/entity/model/HeartBlockContent.c and UltraBlockContent.c, src/entity/HeartBlock.c, src/entity/SuperBlock.c.
## Objective
- FACT (nd_gfx_conv.c:450): the lists load textures from `unsigned char` arrays (HeartBlockContent.c:4-16,
  UltraBlockContent.c:3-12). gen_gfx_syms.py:17 DEF maps only Gfx, Vtx and Vec3s, so the TIMG unit fails with
  `unit address outside the symbol map`. FACT (grep): only these 2 files define a top-level `unsigned char X[] =` in src.
- The patch adds the RAW rule (gen_gfx_syms.py: `unsigned char NAME[] =` gives a GS_IMG entry, end NAME + sizeof NAME).
  Then gset.py moves 12 lists: 6 Heart (with D_0A000758_E4A598), 6 Ultra (FACT oracle dry run: `gset moved 12 files 2 keep 0`).
- FACT (oracle host run, patched tree): ND HOST TESTS PASS, `rawimg ok`, `ntab tables 1944 draws 36485 bad 0`, `gset test ok`.
- Counters (FACT, oracle listcen on the patched tree): data 1183 -> 1114, data_arrays 90 -> 78, gfxtype 580 -> 563.
  build 386, gfxpos 89, recdl 11: no change.
## Edit shapes (the patch holds every edit; for reading only)
- HeartBlock.c:18. Before: `extern Gfx Entity_HeartBlockContent_RenderHeartSleeping[];`
  After: `NDN_DECL(Entity_HeartBlockContent_RenderHeartSleeping);`
- SuperBlock.c:220. Before: `data->gfx1 = Entity_UltraBlockContent_Render;` After: `data->gfx1 = NDN(Entity_UltraBlockContent_Render);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_RAWIMG.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 9 files: `RAWIMG 1: gen_gfx_syms RAW rule (unsigned char arrays as GS_IMG), Heart and Ultra content lists to device keys`.
3. `sh 3ds/tools/rowloop.sh --row RAWIMG --bench entcape_kmr03_drec,spot_mac01_drec --why "RAWIMG gate" > build3ds/out/rawimg_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS (`rawimg ok`, conv2c2 bad 0, gset test ok), build rc 0, both cmp lines byte-equal.
- GUESS: entcape_kmr03_drec draws the heart block content (kmr_03/entity.c:68). FACT (grep): no map spawns the Ultra block,
  so its 6 lists have only the host proof (conv2c2: every draw of every table equals the v1 host model).
- `python 3ds/tools/listcen.py | grep "^listcen total"`: data 1114, data_arrays 78, gfxtype 563 (minus what ENTCAPE moved).
- The seeded fault is in rawimg.sh (RAW_RULE False drops the entry). The bench is not new: no bench self-test.
## What you may edit to fix a fail
Only the 9 files. A DIFF or a base fail: stop. Send build3ds/out/loop_RAWIMG_cmp_<bench>.txt to the oracle.
Budget: 1 build, 4 Azahar starts.
## Stop and hand-back
Last commit `RAWIMG 2: gate. entcape_kmr03_drec, spot_mac01_drec byte-equal; data <n> data_arrays <n> gfxtype <n>`
deletes ws_RAWIMG.md and the patch. One SubagentHandback: commit hashes, the LOOP lines, the listcen totals.
