# DIAG BATEND: world tri and cost rise after a battle (Forever Forest)

Status: code reading only. No Azahar run. Budget used: 0 of 3. Tags: [M] measured, [C] code, [E] estimate.

## 1. Measured input (from diag_forest.md) [M]
- Calm WORLD before battle: 34.5 ms per frame (logstat window f27438 to f27738, 9 closes).
- Post-battle WORLD window f27826 to f28043: 51.7 ms per frame. No logstat or dcsum line covers it.
- Static tris per frame: 30 (f27599), 277 (f27899), 552 (f28199, post-battle).
- Toad Town steady: 923 tris per frame, 48.5 ms. Forest post-battle has fewer tris than Toad. [E]

## 2. Code path of END_BATTLE [C]
- state_init_end_battle (src/state_battle.c:~128) sets BattleTransitionDelay = 5. No reload here.
- state_step_end_battle (src/state_battle.c:134-210), at delay 0:
  - btl_restore_world_cameras, func_8005AF84, func_8002ACDC.
  - init_model_data (line 158): points the model pointers back to the world lists.
  - init_entity_models, reset_animator_list, init_worker_list, init_npc_list, init_entity_data, init_trigger_list.
  - Not map path: partner_init_after_battle, load_map_script_lib, decode_yay0 of wMapShapeName (line 182), initialize_collision, restore_map_collision_data, dma_copy, load_map_bg, set_background.
  - mdl_load_all_textures (line 199) and mdl_calculate_model_sizes (line 200), then npc_reload_all (line 201).
- The battle start path (state_battle.c:83) calls clear_model_data (src/model.c:2488). That function zeroes wModelList, wTransformGroups and the custom GFX lists, and sets *gCurrentModelTreeRoot = nullptr. It also resets fog to 950 to 1000 and clears the texture panner arrays.

## 3. What the world entry path does that the END_BATTLE path does not [C]
- world.c:215 calls load_data_for_models (src/model.c:3509). That function calls load_model_transforms (src/model.c:3526), which rebuilds the model list from the shape tree. It also sets *gCurrentModelTreeRoot.
- The only other caller is src/battle/btl_states_actions.c:235 (battle stage load).
- END_BATTLE never calls load_data_for_models. So the world model list stays empty after the battle, unless another path fills it. No such path found in src/ or 3ds/src/.
- world.c also runs mapConfig->init, create_cameras, clear_effect_data, player_reset_data, and load_map_hit_asset. END_BATTLE runs none of these. The map init (for mim_05, SetupForeverForestFog and the 17 hint panners in hint.c) is not run again. Fog stays at the battle values.

## 4. Conflict between code and measurement [C][M]
- The code says the world model list is empty after END_BATTLE. The measurement says 550 to 590 static tris per frame after END_BATTLE.
- So one of these is true: (a) a 3DS hook (3ds/src/*.c, pm_*) fills the world model list outside the decomp path; (b) the tri count in dlc2 and mdlsum counts something other than the world list; (c) the models drawn after battle are battle-side models (bModelList) that were not cleared.
- The code does not settle which. No 3DS hook for load_data_for_models was found by grep. [C]

## 5. Candidates for the extra 17 ms [E]
1. Model list state after the battle (section 3 and 4). Cause unknown. Cost unknown.
2. DL recompile after reload. decode_yay0 bumps pm_dlc_gen (3ds/src/pm_dlc.inc.c:8, pm_tex.c:857). Compile count was 131 to 203 per 300 frames after battle, 0 in calm. Estimate 0 to 1 ms per frame. Not measured.
3. Texture re-upload (mdl_load_all_textures, model.c:2440). Forest VRAM bank A at load: 3031 KB used, 41 KB free. Cost not measured.
4. Fog and panner state reset (section 3). Changes shading, not tri count. Cost not measured. [E]

## 6. Smallest fix (not applied) [E]
- Only after a measurement names the cause. No fix is proposed yet.
- If candidate 2 is the cause: skip the pm_dlc_gen bump in mdl_load_all_textures or decode_yay0 when the shape hash is unchanged (src/state_battle.c:182, 3ds/src/pm_dlc.inc.c:446). Expected console gain 0 to 1 ms. Risk: stale DL if the hash misses a change.
- If candidate 1 is the cause: call load_data_for_models from END_BATTLE after the texture step (src/state_battle.c:199). Expected gain unknown. Risk: double model lists if a hook already fills them.

## 7. Not done
- battle_return scenario: not added. The azt battle switches did not win a battle in earlier runs (diag_forest.md section 8), so the until-line cannot be verified yet. azt_scenarios.json and test_catalog.md are unchanged from HEAD.
- Azahar: not run.

## 8. Console phase run that settles it
- Run push_3ds.sh with phase, pacestat, logstat and dcsum on the battle path (the user runs it). Read the window f(end+0) to f(end+300).
- Compare models in the world list (count of non-null wModelList entries) before and after the battle. A count of 0 after battle with 550 tris rise points to a hook or battle list (section 4 a or c). A count equal to the calm count points to duplicate models.
- Record dlc2 static count and DL compile count in the same window.
