# N3CRASH: N3DS crash of build 981 (log 3ds/logs/perf/hw_pull5_n3/hw_n3ds_log.txt)

## Result
The crash is not an intro crash and not an N3DS fault. The console loaded the map tst_12 through the Warp-to option (line 135: `warp: tst_12 entry 0 (area 27 map 6) story -107`). Story -107 is STORY_CH0_ARRIVED_AT_TOAD_TOWN, the story value the Warp-to map list and options.txt `warp=tst_12:0` use (3ds/src/pm_warp.c:116-122, 266-268). The switch list (line 12) has no warp switch.

## Chain (build 981 ELF, addr2line)
- pc 0017a914: create_effect_instance, src/effects.c:218. It writes newEffectInst->effectID (offset 4, far=00000004) with newEffectInst = NULL.
- lr 0017a9f0: effects.c:215. This is the malloc line: general_heap_malloc returned NULL.
- Stack: floating_flower_main (src/effects/floating_flower.c:28) <- PlayEffect_impl (src/evt/fx_api.c:405) <- update_scripts (script_list.c:609) <- evt_execute_next_command (evt.c:1671) <- step_game_loop (main_loop.c:88) <- __wrap_step_game_loop (pm_phase.c:335) <- gfxRetrace_Callback (main.c:136) <- pm_frame_loop (pm_main.c:398).
- Log #162: ASSERT i < ARRAY_COUNT(gEffectInstances) (96 slots full). Log #163: ASSERT newEffectInst != nullptr. The port ASSERT only logs. The code goes on and writes gEffectInstances[96] and then reads NULL.

## Root cause (proven)
The script N(D_80240050_B1D340) in src/world/area_tst/tst_12/main.c computes LVarF as a float and then does Add(LVarF, 11) as an integer. The Loop count becomes negative (-17) and the loop never ends. It makes floating flowers until the 96 slots are full. This is the known row F10 (3ds/SLICES.md, QUESTIONS.md Q-F10, docs/HISTORY.md F01 and F02: "tst_12 floods effects, same on N64"). Azahar stops the same way with warp=tst_12:0 (F10 run of 2026-10-19: same assert at effects.c:215, same caller floating_flower_main). So this is not N3DS specific and not FS1: effects are freed by floating_flower_update (remove_effect when timeLeft < 0), which runs in step_game_loop, not in the draw path.

## Differences O3DS and N3DS
None in game logic. The same warp on the O3DS gives the same crash. Build 970 on the O3DS ran Toad Town because it did not load tst_12. The only timing difference is the first frame of 3944 ms (ROM resident read, line 107), which does not matter here.

## Fix
- Option B (default of Q-F10): remove tst_12 from the Warp-to list and from the tour (3ds/src/pm_warp.c pm_warp_map_name / pm_warp_map_count and the options.txt reader pm_options.c:111-119). Not a graphics file, not FS1.
- Option A: patch the script data in src/world/area_tst/tst_12/main.c (AddF for LVarF). It changes decomp data.
- Console action now: delete `warp=` from options.txt on the SD card, or set warp to off.
