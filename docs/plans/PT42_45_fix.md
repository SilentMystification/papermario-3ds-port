# PT42, PT45, PT46, PT47: save block and pipe faults. Result: NOT reproduced in Azahar, no root cause proven

Row INV-SAVE. Build 984 (3ds/logs/builds/pm_3ds_984.3dsx), Azahar, PM_ROW=INVSAVE, 3 starts (all used). Source not edited. Console not used. Evidence level for every cause below: GUESS. No fix text is justified.

## What was run
All runs use `fixedseed loglevel=debug` and the default GV1 and RAW1 (both default on, DLC default off). Route files are in 3ds/logs/perf/invsave/ (copy of sdmc:/3ds/PaperMario/routes/). Logs: r1.txt, r2.txt, r3.txt in the same folder. Shots: the last run only is kept in 3ds/logs/perf/invsave/shots/ (the script wipes the folder at every start). Contact sheets: sheet1.png to sheet4.png.
- Run 1 (PT42 and PT45 try). `PM_ROW=INVSAVE sh 3ds/run_azahar.sh -x 3ds/logs/builds/pm_3ds_984.3dsx -F -t 240 -u "route: seg end" -o 3ds/logs/perf/invsave/r1.txt -d "warp=mac_01:1 story=ch0_toad_town partner=1 route=invsave1 shots loglevel=debug fixedseed"`. Result: Mario stopped at (430, -131) ("leg blocked") on the way to the save block, then went into the Toad House by mistake (door at x 467). One Toad dialog ran inside the house room ("...Really?", nap choice). The save block was not in view and was not hit. No fault seen in the 37 shots.
- Run 2 (PT42 try with A/B states). Same, route=invsave2, `shotwin=60:300 fs1off abtog=base,nogv1,noraw1,nofl1,nodr1:4`. Result: no dialog. The story ch0_toad_town has no Toad at (540, 0): Mario stood at the fence and the route ended after 327 steps. 300 shots, no fault. The abtog string has commas. It broke the column count of 3ds/logs/test_budget.csv, so the budget file counts this start wrongly (the file shows 2 INVSAVE starts, the real number is 3). The abtog log lines (`pm_log_hw`) are not in the Azahar log, so the state of each frame is not known from the log.
- Run 3 (PT45 and PT46 try, Yoshi's village jan_03). `-d "warp=jan_03:2 story=ch0_toad_town partner=1 loadslot=0 route=invsave3 shots shotwin=330:340 loglevel=debug fixedseed fs1off"`. Result: the leg to the block ended at (229, -39) ("leg blocked", twice). The block is at (100, 60, -25). The jump A*24 at x 229 missed the block. No message box, no save, no fault seen in 340 shots (frames 330 to 669). The save block draws as the rainbow S at the upper left of the view.
Conclusion: no fault trigger was reached. The answer "does not reproduce in Azahar" is NOT shown. It is "not tried successfully".

## Facts found by reading (proven by code)
- Save block: src/entity/SaveBlock.c:24-72 `entity_SaveBlock_setupGfx`. Two layers (RenderContent with G_RM_ZB_CLD_SURF, RenderBlock with G_RM_AA_XLU_SURF), prim alpha 128, render task mode XLU layer 3 (line 183). Each layer pushes its own matrix with `G_MTX_PUSH | G_MTX_MUL`. The position comes only from the matrix that `appendGfx_entity_model` loads first (src/entity_model.c:250-251, `G_MTX_PUSH | G_MTX_LOAD`, from `model->transform`, a copy of `entity->transformMatrix` made at queue time, entity_model.c:423). `Entity_SaveBlock_Mtx` is the identity (src/entity/model/SaveBlock.c:67) and is only read.
- A block drawn "at the center of the area" is a block drawn with the identity model matrix, because the block model is centred on its own origin and the world origin is the centre of the map. This fits PT42 better than a wrong entity position. The entity position (entity->pos) is not changed by a dialog in the decomp code (src/entity.c:478-505, only scale and y change in the hit script, Block.c:41-61).
- Pipe: Entity_BlueWarpPipe (src/entity/BlueWarpPipe.c:147-180). The pipe model is hidden by sinking it 52 units under the floor (`entity->pos.y -= 52`, line 179) when its flag is not set. The visible flat pad is the base drawn by setupGfx with `G_MTX_PUSH | G_MTX_LOAD` at `finalPosY + 1` (line 155-158). The pipe is at (-540, 0, 195) in jan_03 (src/world/area_jan/jan_03/entity.c:63), the save block at (100, 60, -25) (line 64). The pipe is far outside the view when Mario stands at the block. A "pipe" that flickers near the save block is therefore NOT the real pipe at its own place. It is a draw of the pipe model (or of its texture) at a wrong place or a wrong matrix. This supports the same cause as PT42 and PT45 (a stale or wrong matrix or texture applied to an entity draw, for a frame).
- The port keeps a per frame table of game matrices (3ds/src/pm_gbi.c:260-261 `gm_tab`, 256 entries, search at 2263-2269) and a palette of 21 matrices per raw batch (3ds/src/pm_gpu.c:106 `GPAL`, filled in `pm_gpu_gtrip` 1502-1570, uploaded in flush 455-481 with the RAW1 shadow `sh_pal_*` at 476). The palette index is stored in the Vtx flag field of the ring copy (pm_gpu.c:1551, 1474). Every dialog or save adds draws (message window, text, cursor), so these tables see their highest load in the frames of PT42, PT45 and PT46.
- F06 (pm_interp.c) is OFF by default (`fps60` switch or option). The user build list does not name it. F06 is not the cause unless fps60 was on. Check on the console: Options row fps60.
- The mv matrix stack in the interpreter drops a push when 10 levels are used (pm_gbi.c:1749 `mv_sp + 1 < 10`) without a log line. The following pop (pm_gbi.c:2429) then pops one level too many. A frame that leaves one level open (an unbalanced push in an earlier task) would make the next entity draw lose its matrix or its parent. N64 code has the same limit, so the decomp is balanced. The port has no log for this case.

## Causes, ranked (all GUESS, none shown by a run)
1. GV1 palette or game matrix index slip in a frame with a high matrix count (pm_gbi.c:2263-2269, pm_gpu.c:1516-1563, 467-481). Symptom match: wrong matrix on one entity draw for the length of the dialog (PT42), at save end (PT45), when the save block is hit (PT46). A/B: `nogv1`. Not tested.
2. RAW1 lean shadow (`sh_pal_ok`, `sh_pal_gm`, `sh_pal_gen`, `sh_ok`, `bound`, `buf_cur`, pm_gpu.c:113-117, 459-481) is cleared only at frame begin (`raw1_reset`, pm_gpu.c:1352) and at an abtog switch. `sh_clear()` (pm_gpu.c:345), which runs at every C3D_FrameSplit (KICK1, grab split, pm_gpu.c:265, 404, 1258) and at every off-screen target switch (1176, 1189), does NOT clear them. If the hardware (or Azahar) loses the uniform registers or the bound program after a split, the lean path skips an upload that is needed. A dialog frame has more splits (grabs, off-screen text effects). This also fits "console only". A/B: `noraw1`. Not tested. Needs a check of what citro3d does with the uniform registers at C3D_FrameSplit and C3D_FrameDrawOn.
3. Texture slot mix-up: the S letter texture (64 x 32 IA8) or the pipe texture bound to a UI quad (FL1 slab free list, pm_tex.c:297-445, or the PG1 "same texture again" shortcut, pm_gbi.c:2921-2938). The dialog loads many glyph textures. This would show an S or pipe picture at the screen centre, not a moved block. The user words ("the block moves") need a photo to tell the two apart. A/B: `nofl1`.
4. FS1 timing on the console: a long step (flash save, dialog start) makes FS1 skip draws. A skipped draw does not run the draw side state of the interpreter. Not shown. A/B: `fs1off`.
Not a cause (read): FS1 for the camera (PT26_fix.md), F06 (off), DLC (off), entity heap overlap for the save block (ENTITY_ADDR is the plain pointer on the 3DS, include/entity.h:67).

## Next test (needs 3 starts, one agent, nothing else on Azahar)
Fix the routes first. The facts:
- mac_01 entry 1 is (600, 0). The way to the block (280, 80, -130) is blocked at (430, -131) and the Toad House door is at x 467 near z -160: walk z = -60 first, then west. Look at 3ds/logs/perf/invsave/sheet2.png: the block is on the wall of the post office side. Find the real path with padlog (`padlog` writes Mario's position every 2 frames).
- A Toad to talk to: the story ch0_toad_town has no Toad at (540, 0). Use the Toad House keeper (488, 20, -174) from inside the room, or pick a story with `story=-100` and a Toad from src/world/area_mac/mac_01/npc.c:2102-2229 (Toad_04 (-94, 0, -117), Toad_05 (-145, 0, -25), Toad_09 (-380, 20, -100)). The talk starts with A in front of the Toad.
- jan_03: Mario is stopped at (229, -39) from entry 2 (583, 0). Try entry 0 (-583, 0) or go_229_60 first. Block (100, 60, -25): stand at (101, -18) and hold A for 24 scans.
Commands (no commas, so the budget file stays correct): one state per start.
- Start 1: default switches plus `fs1off shotwin=<hit frame - 20>:120`. Find the first frame with a wrong block or pipe.
- Start 2: the same with `nogv1`.
- Start 3: the same with `noraw1`. If the fault stays in both, run `nofl1 nodr1 nomerge` together and then read the draw log (below).
Draw log of the bad frame: `drawlog=<f>:1` writes one line per mesh chunk with the image address. The S texture is in the entity data (3ds/logs/builds/pm_3ds_984.map: Entity_SaveBlock_RenderBlock 0x005e8838, Entity_SaveBlock_Mtx 0x005e85a8, Entity_BlueWarpPipe_RenderPipe 0x005e79b8). A chunk with these sources but a vertex position or matrix that is not the entity place shows the fault.

## Debug patch text (not applied, for the graphics lane)
1. src/entity.c, in `render_entities`, inside `if (!(entity->flags & ENTITY_FLAG_HIDDEN))` before `FI_OPEN("entity", ...)`:
```
#ifdef TARGET_3DS
    if (pm_debug_has("entlog")) {
        s32 t = get_entity_type(entity->listIndex);
        if (t == ENTITY_TYPE_SAVE_POINT || t == ENTITY_TYPE_BLUE_WARP_PIPE) {
            Matrix4f em;
            guMtxL2F(em, &entity->transformMatrix);
            pm_log_d("entlog f%u t%d i%d pos %.1f %.1f %.1f tm %.1f %.1f %.1f sc %.2f %.2f %.2f al %d fl %x mlp %d", pm_frame_count(), t, i,
                     entity->pos.x, entity->pos.y, entity->pos.z, em[3][0], em[3][1], em[3][2],
                     entity->scale.x, entity->scale.y, entity->scale.z, entity->alpha, entity->flags, gMatrixListPos);
        }
    }
#endif
```
(declare `extern int pm_debug_has(const char*); extern void pm_log_d(const char*, ...); extern unsigned pm_frame_count(void);` at the top of the block.) A frame where `tm` is 0 0 0 or `pos` differs from `tm` is the fault.
2. 3ds/src/pm_gbi.c, in `load_mtx` after the `unpack` (line 1747), under a static switch flag `mtxlog` read once with pm_debug_has:
```
if (mtxlog) pm_log_d("mtx f%d src %p w0 %02x sp %d t %.1f %.1f %.1f", rm_frame, (void*)src, w0 & 0xff, mv_sp, in[3][0], in[3][1], in[3][2]);
```
and in the `if (!is_proj && (flags & G_MTX_PUSH) ...)` branch an `else if` that logs `mtx push dropped f%d` when `mv_sp + 1 >= 10`. Use only with a `shotwin` of a few frames (the log is large).
3. PT26 camera patch (`camlog`) as written in docs/plans/PT26_fix.md, for PT47 and PT05.

## PT47 (Kolorado attacked, text effect, camera snap)
Not run. The scene is jan_01 (src/world/area_jan/jan_01/npc.c:117-140, `EVS_NpcHit_Kolorado`: DisablePlayerInput, SpeakToPlayer with ANIM_Kolorado_Shout, MSG_CH5_000E) or jan_22 (`EVS_NpcHit_Kolorado_HeldCaptive`, npc.c:971-995, MSG_CH5_00BD, with ShakeCam calls at npc.c:286-355). Both scripts have no camera command in the hit part. `SpeakToPlayer` (src/evt/msg_api.c:20-140) only reads the camera (get_screen_coords) and changes no camera state. So the camera snap is not from the script. This is the same result as PT05 (PT26_fix.md). The common part of PT05 and PT47 is a message with a text effect (shout style) and a shake. The camera snap is probably the zone camera (jan_01 and mac_01 both use a zone camera) or a draw-side effect of the message window. Same A/B switches as above apply. No root cause. Hardware evidence needed: the `camlog` patch from PT26_fix.md and a log of the run with `loglevel=debug`.

## Hardware evidence for the user or the console agent
1. Build, options and switches of the faulty run (fps60 on or off, wide on or off, FS1 state).
2. For PT42: the map and the Toad (name or position), and one photo with the block at the wrong place.
3. A run with `entlog` and `mtxlog` (after the patch above) and `loglevel=debug`, pulled with pull-o: the lines of the frames around the dialog start. The line pair (entlog tm, mtx t) tells if the entity matrix is wrong (game side) or the draw uses another matrix (port side).
4. The same run with `nogv1`, then with `noraw1` (two more pushes, one switch each). A fault that stays with both is not in GV1 or RAW1.
