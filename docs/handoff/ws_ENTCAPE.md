# ws_ENTCAPE (Haiku): the 10 entity list sites get a captured entry (nd_draw_list + ND_PSO_DESC). C1 -10, C2 -10 at most
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTCAPE, PM_ROW=ENTCAPE PM_LANE=ENTCAPE. Main at release: bd56f6cec9.
Start: after `Merge ENTSRC` (switch entcapt, entapply SITE, ntab_ent.d). Files: SaveBlock.c, HeartBlock.c, SuperBlock.c,
sbk_omo/Tweester.c, BlueWarpPipe.c, jan_iwa/Plants1.c in src/entity. No other page edits them.
## Objective
- Sites (FACT grep): SaveBlock.c:42, HeartBlock.c:54 :69, SuperBlock.c:150 :165 :180, Tweester.c:43 :61, BlueWarpPipe.c:160,
  Plants1.c:177 (entity_PinkFlowerLight_setupGfx). Their lists are itab tables (FACT ntab_inherit.txt, post-merge 43b35ab465).
- Form: capture form `nd_rec_dl(X);`, an Azahar capture (entcap entcapt), then entapply.py writes `nd_draw_list(X, pe_<stem>_<line>);`
  and the ND_PSO_DESC line. gen_ntab_entries.py reads the files of ntab_ent.d/ENTCAPE.txt (converter seeds).
- Capture tour (GUESS each: the entity is in view): kmr_02 125 0 300 (pipe 0 0 355, save block 250 60 75: kmr_02/entity.c:140-141),
  kmr_03 tour spot 96 0 96 (heart block 130 60 0: kmr_03/entity.c:68), flo_08 -780 0 40 (super block -780 120 -110: flo_08/entity.c:9),
  sbk_14 entry 3 (sbk14_drec scene), jan_06 84 0 -170 (pink flower 84 0 -320: jan_06/entity.c:5). Until `tour: done items 5` (pm_warp.c:1462).
## Design (docs/handoff/ws_ENTCAPE.patch, 9 files: the 6 sources, ntab_ent.d/ENTCAPE.txt, azt.d/ENTCAPE.json, bench.d/ENTCAPE.json)
New benches: entcape_cap (log only, the tour) and entcape_kmr03_drec (gate, drec=200:2, until `drec f201 ...`, pm_gpu.c:618).
FACT (oracle, 023db9ca81): `git apply --check` OK. listcen on the patched tree: build 376, gfxpos 79, recdl 21 (capture form).
## Edit shape (SaveBlock.c:42). Capture form: `    gSPDisplayList(gMainGfxPos++, dlist);` -> `    nd_rec_dl(dlist);`
After entapply: `    nd_draw_list(dlist, pe_SaveBlock_42);` and after `#include "nd_gtab.h"`: `ND_PSO_DESC(pe_SaveBlock_42, 0x..., ...);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTCAPE.patch | git apply -`. Commit the 9 files: `ENTCAPE 1: capture form of 10 entity sites`.
2. `sh 3ds/build.sh > build3ds/out/entcape_b1.txt 2>&1`. Then `python 3ds/tools/bench.py run entcape_cap --row ENTCAPE --why "ENTCAPE capture"`.
3. Log path L = the `log_path` of the last line of 3ds/logs/perf/runs.jsonl with `"bench": "entcape_cap"`.
   `python 3ds/tools/entcap.py L > build3ds/out/entcape_cap.txt` (newest ELF of your build). Then
   `python 3ds/tools/entapply.py build3ds/out/entcape_cap.txt > build3ds/out/entcape_apply.txt`.
4. For each of the 10 sites: `entapply site <file>:<line>` or `entapply so`: keep. Any other case (no line for the site, multi, skip,
   only `nt` lines in entcape_cap.txt): put the original line back (`gSPDisplayList(gMainGfxPos++, <arg>);`). Name it in the hand-back.
5. Commit: `ENTCAPE 2: captured entries of <n> entity sites`. Run `python 3ds/tools/bench.py base entcape_kmr03_drec`.
6. `sh 3ds/tools/rowloop.sh --row ENTCAPE --bench entcape_kmr03_drec,sbk14_drec,spot_mac01_drec --why "ENTCAPE gate"`.
## Tests that decide
- `LOOP PASS` (host PASS, build rc 0, 3 cmp byte-equal). listcen: build and gfxpos fall by n (applied sites), recdl as main (11).
- build3ds/out/ntab_entries.txt has a line with `src/entity/` for each applied site except SuperBlock (FACT dry run: `data->gfx2`
  gives `gne skip`) and HeartBlock (the list is the parameter arg1). Record the gconv line of loop_ENTCAPE_build.txt.
- Seeded fault: HeartBlock.c:44 `alpha = 92;` -> `alpha = 93;` (uncommitted), `rowloop.sh --row ENTCAPE --bench entcape_kmr03_drec
  --no-host`: cmp DIFF. Undo with git checkout of the file.
## What you may edit to fix a fail
Only the 9 files. A DIFF of the real edit: put back the original line of the site whose file the DIFF frame draws, 1 try. Then stop
and send build3ds/out/loop_ENTCAPE_cmp_<bench>.txt to the oracle. Budget: 3 builds, 7 Azahar starts.
## Stop and hand-back
Last commit `ENTCAPE 3: gate. <benches> byte-equal; fault DIFF; build <b> gfxpos <g>` deletes ws_ENTCAPE.md and the patch.
One SubagentHandback: commits, applied and reverted sites with the reason, LOOP lines, listcen totals.
