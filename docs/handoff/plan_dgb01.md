# plan_dgb01.md: dgb_01 lag with the broken bridges (oracle DGB01, code and asset read, edit in place)
User report (FACT, 21 Nov): "DGB_01 area 15 map 1 is super laggy when the bridge is broken". Area 15 is AREA_DGB (include/enums.h:3012), map 1 is dgb_01 (src/world/world.c:707).
Tags: FACT (file:line or asset parse), GUESS (estimate), OPEN (not measured). Line numbers: main 3a29e12504.

## 1. What the broken state changes (FACT code read)
| item | intact | broken | file:line |
|---|---|---|---|
| story selector | story < -29, or story >= -16 | -28 to -17 (STORY_CH3_TUBBA_SMASHED_THE_BRIDGES to before DEFEATED_TUBBA_BLUBBA) | bridges.c:180-194, enums.h:110, :122 |
| shown bridge group | g340 (MODEL_g340 0x70) | g240 (MODEL_g240 0xD2) | bridges.c:183, :190, dgb_01_shape.h:18, :116 |
| collision | COLLIDER_after ignored | COLLIDER_before ignored | bridges.c:184, :191 |
| music | SONG_TUBBAS_MANOR | SONG_TUBBA_ESCAPE | music.c:6, :8 |
| NPCs at entry 0, 1, 3, 5 | 4 Sentinels (story < -29), none (story >= -16) | none | main.c:180-193 |
| NPCs at entry 2, 4, 6 | as above | 4 Sentinels (static) and Tubba. Entry 4 adds an endless ShakeCam thread while GB_ARN_Tubba_MapID is 1 and story is -28 | main.c:185-192, npc.c:94-117 |
- Story -29 (TUBBA_WOKE_UP) runs the collapse scene (bridges.c:185-188): an animated model of the bridges (bridges.c:74) until bridges.c:153. This is a cutscene only (OPEN cost).
- Asset parse (FACT: tree walk of assets/us/mapfs/geom/dgb_01_shape.bin, the method of 3ds/tools/tour_spots.py read_shape, node id = post-order index = MODEL_ value):
  g240 has 84 leaf models, 2000 triangles, 2783 vertices, 7 textures, render mode 1 (66 models) and 13 ALPHATEST (18 models). 20 models of g333 have 2 triangles each.
  g340 has 16 leaf models, 208 triangles. The map has 169 leaf models, 3485 triangles, 13 textures. No group in g240 or g340 has a transform matrix. The map has no fog script.
- So the map draws 153 models and 3277 triangles when broken, against 85 models and 1485 triangles when intact (FACT arithmetic, before the bounds culling of src/model.c:2452-2522).

## 2. The path of these models on the native draw path (FACT code read)
- Each shown map model is one model record (appendGfx_model, src/model.c:2533-2536). Core 1 records it (c1d rmdl, snap). Core 0 runs rec_run (nd_recs.c:274-302).
- Hit: key (NDT_MKEY), rec_matrix, pm_gpu_model_hit (pm_gpu.c:4357-4411): texture lookup, template touch, nd_mtx_combine and pcomp, pm_gpu_flush (:4394), one submit per draw (:4406).
- One model gives one draw: nd_model_draw merges ranges only inside one model (nd_model_set.c:46). So the broken state adds about 68 records and 68 PICA draws per frame (GUESS: culling not counted).
- No cliff in the cache by size: 1024 entries, 512 pool draws (nd_model.c:475-476), trim at 3/4 (:613), clear only when the live set passes 5/8 or 7/8 (:610). 153 entries fit (FACT arithmetic).
- No texture cliff: the broken group adds 2 textures (dgb_tesuri2tif, dgb_tesuri3tif) to 11 (FACT asset parse).
- Unit costs (FACT plan_critpath.md 2.1, mac_01): core 0 model path 75 us per model, submit 57 us per draw. Projection: +68 x 132 us = +9 ms core 0 per step (GUESS).
  ft 41 ms on mac_01 (FACT plan_critpath.md 0) would then be about 50 ms (GUESS).

## 3. Ranked causes and the counter that decides
| # | cause | proof now | decides |
|---|---|---|---|
| 1 | +68 model records and draws per frame: linear core 0 cost (record, hit, submit), core 1 cost (rmdl, snap, game draw code) | FACT model counts (1), FACT path (2) | Azahar ndc model and pipe draws pair delta. Console: cls rec, sub, c0r hit, c1d rmdl, ft pair delta |
| 2 | a broken model that does not hit (refusal or key change each step): miss path every frame | OPEN | Azahar stepx norm m and nd leg (pair) |
| 3 | K copy refused (kwalk): core 0 walks K again | OPEN (pm_gpu.c:3225-3236 causes) | Azahar kcen kwalk (pair) |
| 4 | music SONG_TUBBA_ESCAPE on core 1 (audio threads) | OPEN | console core2 lgms, wait au, mx (pair) |
| 5 | collapse scene animated model (story -29 only) | OPEN | not measured (cutscene) |

## 4. Rows
| row | model | what | page |
|---|---|---|---|
| DGB0 | Haiku | measurement: scenario and bench pair dgb01_broken_prof (story -28) and dgb01_whole_prof (story -16), capture_check of both console words | ws_DGB0.md, start now |
| DGB1 | - | page after DGB0, by rule D1 to D3 below | not written |
Rules after DGB0 (oracle):
- D1: broken sx_m > whole sx_m: cause 2. Next page: keymiss census on story -28 (keymiss switch, pm_keymiss.c).
- D2: broken nd_leg > whole nd_leg: cause 2 (refusal). Next page: ndmcheck census of the pair (why codes, pm_gpu.c:4422).
- D3: broken kc_walk > 0: cause 3. Next page: kcen why split.
- D4: none of D1 to D3, and ndc_model delta 30 or more: cause 1. The orchestrator asks the user for the console pair (section 5). The fix is per-model cost:
  L1C0B (core 0 hit queue, oracle L1: dgb_01 has long hit runs of static models) and the static-batch design (many small static models with one
  texture and no matrix in one draw: design pass, Opus, needs the user's yes). Add dgb01_broken_prof to the L1C0B gate as a count bench.
## 5. Console capture (orchestrator, on the user's word only, after the capture_check of DGB0)
- Broken: `powershell -NoProfile -File 3ds/go.ps1 push o -ip <addr> -NoBuild -Seconds 150 -Profile prof -GameDebug "warp=dgb_01:1 story=-28 pos=80:0:144 profdisk=1"`
- Intact: the same words with `story=-16`. Proof lines: `at dgb_01 15:1`, `sp-28` (or `sp-16`), `ndc model`, `stepx norm`.
- Entry 1 is the west door. Entry 0 is not used: its first visit sets the story to -32 (main.c:120-122). pos 80 0 144 is the tour spot (3ds/tours/spots.txt:290).
- Read per pair: ft, wk, cls rec, sub, c0r hit, c1d rmdl, core2 lgms, ndc model, pipe draws. Compare only windows with equal ndc lines (ws_CAPCMP rule).
