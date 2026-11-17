# ws_ENTCAPR (Haiku): 5 root and world list sites get a captured entry (curtains, file select cursor, Merlee cards). C1 -5, C2 -5 at most
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTCAPR, PM_ROW=ENTCAPR PM_LANE=ENTCAPR. Main at release: 743c94e8ff.
Start: after `Merge ENTSRC`. Files: src/curtains.c, src/filemenu/filemenu_common.c, src/world/area_dro/dro_02/npc_merlee.c.
No other page edits them (the converter-refused curtains lines :136 :180 :183 belong to the other oracle: do not touch them).
## Objective
- Sites (FACT grep): curtains.c:155 NDN(TheaterGfx), filemenu_common.c:996 and :1018 D_8024B708 (filemenu_draw_cursor),
  npc_merlee.c:248 NDN(N(card_1_gfx)), :259 NDN(N(card_2_gfx)). Each list is an itab table (FACT ntab_inherit.txt, build 43b35ab465).
- Form as ws_ENTCAPE: capture form `nd_rec_dl(X);`, Azahar captures with `entcap entcapt`, entapply.py, ntab_ent.d/ENTCAPR.txt
  (npc_merlee.c with `=dro_02`: N(x) is dro_02_x, dro_02.h:41).
- Captures (one start each; the words of curt_title_drec, wscmb_fsel_drec and gmerlee_drec plus the 2 switches; until lines
  `drec f540`, `drec f301`, `drec f835`, pm_gpu.c:618): entcapr_curt_cap, entcapr_fsel_cap, entcapr_merlee_cap.
  GUESS: the file select draws its cursor by frame 301.
## Design (docs/handoff/ws_ENTCAPR.patch, 6 files: 3 sources, ntab_ent.d/ENTCAPR.txt, azt.d/ENTCAPR.json, bench.d/ENTCAPR.json)
FACT (oracle, 023db9ca81): `git apply --check` OK. listcen on the patched tree: build 381, gfxpos 84, recdl 16 (capture form).
## Edit shape (npc_merlee.c:248). Capture form: `gSPDisplayList(gMainGfxPos++, NDN(N(card_1_gfx)));` -> `nd_rec_dl(NDN(N(card_1_gfx)));`
After entapply: `nd_draw_list(NDN(N(card_1_gfx)), pe_npc_merlee_248);` and `ND_PSO_DESC(pe_npc_merlee_248, ...);` after the include.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTCAPR.patch | git apply -`. Commit the 6 files: `ENTCAPR 1: capture form of 5 sites`.
2. `sh 3ds/build.sh > build3ds/out/entcapr_b1.txt 2>&1`. Run each: `python 3ds/tools/bench.py run <cap> --row ENTCAPR --why "ENTCAPR capture"`.
3. For each capture: log path = `log_path` of the last runs.jsonl line of that bench. `python 3ds/tools/entcap.py <log> >>
   build3ds/out/entcapr_cap.txt`. Then `python 3ds/tools/entapply.py build3ds/out/entcapr_cap.txt > build3ds/out/entcapr_apply.txt`.
4. Each site: `entapply site` or `entapply so`: keep. Any other case: put the original line back. Name it in the hand-back.
5. Commit `ENTCAPR 2: captured entries of <n> sites`.
6. `sh 3ds/tools/rowloop.sh --row ENTCAPR --bench curt_title_drec,wscmb_fsel_drec,gmerlee_drec --why "ENTCAPR gate"`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, 3 cmp byte-equal. listcen build and gfxpos fall by n, recdl as main (11).
- build3ds/out/ntab_entries.txt: one line per applied site with its list (TheaterGfx, D_8024B708, dro_02_card_1_gfx, dro_02_card_2_gfx).
## What you may edit to fix a fail
Only the 6 files. A DIFF of the real edit: put back the site lines of the file that the bench draws, 1 try. Then stop and send
build3ds/out/loop_ENTCAPR_cmp_<bench>.txt to the oracle. Budget: 2 builds, 6 Azahar starts.
## Stop and hand-back
Last commit `ENTCAPR 3: gate. <benches> byte-equal; build <b> gfxpos <g>` deletes ws_ENTCAPR.md and the patch.
One SubagentHandback: commits, applied and reverted sites with the reason, LOOP lines, listcen totals.
