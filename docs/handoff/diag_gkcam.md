# diag_gkcam: Goomba King area, camera rotation, elements rotate then vanish

Tags: [CONFIRMED] = read in code or in a log. [INFERRED] = a reasoned link, not measured.
No source edit. No Azahar start. Row GKCAM, diagnostic budget 3 (0 used).

## 1. Map, battle, cutscene

- Goomba King battle (the "goomboss" fight): formation BTL_KMR_2_FORMATION_01, stage BTL_KMR_2_STAGE_04, story value -112 (3ds/src/pm_boss_table.h line 16). The name "goomboss" is not in the port tree. [CONFIRMED]
- Field map with the fortress exterior and the rotating camera: kmr_11 (reference/paperboat/src/world/area_kmr/kmr_11/scenes.c). The scene is EVS_Scene_MeetGoombaKing (line 458), with EVS_Scene_MeetGoombaKing_Impl (316). Camera calls: FocusCam_FortressExterior (46), FocusCam_BesidePlayer (58), FocusCam_Battlement (70). SetCamSpeed 90.0 at lines 432 to 447 and 460. [CONFIRMED]
- The scene runs only while GB_StoryProgress is below STORY_CH0_DEFEATED_GOOMBA_KING (scenes.c line 118). [CONFIRMED]
- Scenario: none in 3ds/tools/azt_scenarios.json. No preset for kmr_11 in 3ds/src/pm_warp.c (preset table near line 36). [CONFIRMED]
- Repro string to write: `warp: kmr_11 entry 0` with the story progress below -112 (needs a preset row or a save at that progress). [INFERRED: the preset row form is not checked]

## 2. Draw path for the camera-dependent elements

- Billboard: model.c 2044 to 2045 emit G_MTX of gCameras[gCurrentCamID].mtxBillboard. This matrix is guRotate(-curBoomYaw) built each frame in src/cam_main.c 221 to 223. It is the only camera-dependent model matrix found. [CONFIRMED]
- Plain model: model.c 1994 and 2005 emit gSPMatrix(model->finalMtx). No camera term. [CONFIRMED]
- SR2 static matrix cache (commit 70a34af9a): load_mtx in 3ds/src/pm_gbi.c 1767 to 1785. The cache needs header bit 0x100. model.c 1519 clears bit 0x100 for BILLBOARD models. A billboard therefore never reads or writes the cache. [CONFIRMED]
- SR1 version counter (db05981f1): pmMtxVer is set at model.c 2712 (creation) and 2830 (static refresh). The only savedMtx writers are in model.c (2704 to 2707, 2829, 2838). None is outside model.c. [CONFIRMED]
- Group savedMtx (model.c 2891, 3773) has no version bump. The group G_MTX (model.c 3238, 3271) is emitted outside a G_PM_MODEL header, so load_mtx gets hdr 0 and no cache. [CONFIRMED]
- Display list replay (DLC): dlc_compile in 3ds/src/pm_dlc.inc.c (lines 104 to 175) returns DLR_OP for any op not in its list. G_MTX is not in the list. A list with a billboard matrix is never replayed. [CONFIRMED]
- Group box test: dlc_box_out (pm_dlc.inc.c 305 to 322), called at line 481 with `combined`, which is built each frame. Not stale. [CONFIRMED]
- Vertex matrix id: gv_matrix_id (pm_gbi.c 1395 to 1415) compares the full matrix with memcmp. Not stale. [CONFIRMED]
- Foliage transform (src/common/FoliageTransform.inc.c): sets MATRIX_DIRTY only on the first call. Not camera-dependent. [CONFIRMED]
- Interpolation (F06-2, fps60 mode only): load_mtx calls pm_interp_find(src) (pm_gbi.c 1771) before the cache. A guRotate billboard matrix is not an F2L leaf, so the lookup should not replace it. [INFERRED]

## 3. Finding on SR2

SR2 does not touch billboard matrices (section 2). SR2 can only change a non-billboard static model whose matrix changes without MATRIX_DIRTY. Camera rotation does not change such a matrix. SR2 is therefore an unlikely cause. [INFERRED]

The failing element type (billboard) is the camera-dependent path. The exact failing line is not found. [INFERRED: billboard path, not measured]

## 4. Test that separates the causes (not run, no edit)

- Test A (SR2 off): in pm_gbi.c line 1774 set `cached = 0;`. Build, then one warp to kmr_11. If the elements rotate and stay, SR2 is the cause. If the picture is the same, SR2 is not the cause. Cost: one build, one Azahar start.
- Compare with the parent commit db05981f1 (SR1, no SR2 header bits and no cache). Test A is the cheaper switch.
- Billboard path: no minimal change is known until Test A is run. Candidate reads: model.c 2044 to 2045 (billboard G_MTX) and cam_main.c 221 to 223 (matrix build per camera).

## 5. Open items

- The console build tested is not named in the report. Check whether SR2 was in that build (commit 70a34af9a on or before the console push).
- Wide mode (commit 6c59757fe) changes the view. Check whether wide was on during the console test.
- No Azahar run. The symptom is not reproduced here.
