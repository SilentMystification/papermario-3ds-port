# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| HOSTDIR | Haiku | 3ds/tools/nd_host_test.sh one new line after `/tmp/kcc` (:73); new 3ds/tests/host.d/README.txt, 00_hostd.sh | ready (first: the 5 rows below wait for it) |
| DEADDEL1 | Haiku | src/draw_img_util.c :216-343, :359-530; src/background_gfx.c :324-344; src/effect_utils.c :59-111; src/model.c comment block to :4897 | ready |
| BOTHUD | Haiku | 3ds/src/pm_log.c sink_main wait line (:888); 3ds/src/pm_gpu_ui.c pm_gpu_hud_flush counter; 3ds/src/pm_main.c probe next to shotbottom (:813-818); new azt.d/bench.d BOTHUD.json | ready |
| N64DEL2 | Haiku | 3ds/tools/unifdef3ds.py condition regex, 3ds/tools/ppcmp.sh gcc line; TARGET_3DS lines of include, src/world, src/entity, src/battle, src/pause, src/filemenu, src/audio, src/evt, src/os | ready |
| N64DEL3 | Haiku | TARGET_3DS lines of src/*.c (root only) | waits for N64DEL2 |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
| ENTRYCAP | Haiku | 3ds/src/nd_gtab.c nd_rec_dl :84-85; nd_walk.c nd_gtab_run top of the non-state-only arm; new 3ds/tools/entcap.py, azt.d/bench.d ENTRYCAP.json | waits for CONV2D2 (nd_ntab_find, same nd_gtab_run) |
| CODEMODFX10 | Haiku | src/effects/shimmer_burst.c (value call lines, include lines); new azt.d/bench.d CODEMODFX10.json | ready |
| CONV2D2 | Haiku | nd_gtab.h NdNIdx and 2 prototypes; nd_gfx_conv.c v2_convert_all device index; nd_gtab.c inside the ND_SCOMP_HOST guard, gtr_cen; nd_walk.c before nd_gtab_run, :500 and :601; azt.d/bench.d CONV2D2.json | waits for CONV2D |
| CONV2F | Haiku | nd_gtab.h after the nd_rec_dl prototype; nd_gfx_conv.c main argc line, v2_convert_all signature and table loop; gen_gfx_tab.sh :18, :23; codemod.py argv loop, :204, :223; src/effects/water_splash.c:189 | waits for CONV2D2 |
| CODEMODW1 | Haiku | src/world/area_mac/mac_00/water_gfx.c value call lines, include line; new azt.d/bench.d CODEMODW1.json | parked (CODEMODW1 trace) |
| GOLDWLD0 | Haiku | new cov.d/azt.d/bench.d GOLDWLD0 files | waits for COVMAP and the user's fortune scene |
| N64DEL4 | Haiku | the 9 skip blocks: include/PR/os_convert.h, common.h, common_structs.h, include_asm.h, macros.h, src/entity.c, kzn2 lava_piranha.c, WaterEffects.inc.c, lava_piranha/part1.inc.c | waits for N64DEL2 and N64DEL3 |
| GMHARN | Haiku | new 3ds/tools/gold.py; covsites.py argument parsing; new cov.d/bench.d GMHARN files | waits for COVMAP2 |
| GOLDFX2 | Haiku | new cov.d/azt.d/bench.d GOLDFX2 files | waits for GMHARN |
| CODEMODFX11 | Haiku | src/effects/snaking_static.c (value call lines, include lines); new azt.d/bench.d CODEMODFX11.json | ready |
| CODEMODFX12 | Haiku | src/effects/effect_3D.c (value call lines, include lines); new azt.d/bench.d CODEMODFX12.json | ready |
| CODEMODFX13 | Haiku | src/effects/snowfall.c (value call lines, include lines); new azt.d/bench.d CODEMODFX13.json | ready |
| CODEMODFX14 | Haiku | src/effects/pink_sparkles.c (value call lines, include lines); new azt.d/bench.d CODEMODFX14.json | ready |
| CODEMODFX15 | Haiku | src/effects/radiating_energy_orb.c (value call lines, include lines); new azt.d/bench.d CODEMODFX15.json | ready |
| CODEMODFX16 | Haiku | src/effects/quizmo_stage.c (value call lines, include lines); new azt.d/bench.d CODEMODFX16.json | ready |
| CODEMODFX17 | Haiku | src/effects/peach_star_beam.c (value call lines, include lines); new azt.d/bench.d CODEMODFX17.json | ready |
| CODEMODFX18 | Haiku | src/effects/whirlwind.c (value call lines, include lines); new azt.d/bench.d CODEMODFX18.json | ready |
| CODEMODFX19 | Haiku | src/effects/landing_dust.c (value call lines, include lines); new azt.d/bench.d CODEMODFX19.json | ready |
| CODEMODFX20 | Haiku | src/effects/damage_indicator.c (value call lines, include lines); new azt.d/bench.d CODEMODFX20.json | ready |
| CODEMODW2 | Haiku | src/world/area_iwa/iwa_01/splash.c value call lines, include lines; new azt.d/bench.d CODEMODW2.json | parked (CODEMODW1 trace) |
| CODEMODW3 | Haiku | src/world/area_flo/flo_10/waterfx.c value call lines, include lines; new azt.d/bench.d CODEMODW3.json | parked (CODEMODW1 trace) |
| CODEMODW4 | Haiku | src/world/area_hos/hos_04/fountains.c value call lines, include lines; new azt.d/bench.d CODEMODW4.json | parked (CODEMODW1 trace) |
