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
| CODEMODFX2 | Haiku | src/effects/star.c (value call lines, include lines) | ready |
| CODEMODFX3 | Haiku | src/effects/explosion.c (value call lines, include lines) | ready |
| CODEMODFX4 | Haiku | src/effects/lens_flare.c (value call lines, include lines) | ready |
| ENTRYCAP | Haiku | 3ds/src/nd_gtab.c nd_rec_dl :84-85; nd_walk.c nd_gtab_run top of the non-state-only arm; new 3ds/tools/entcap.py, azt.d/bench.d ENTRYCAP.json | waits for CONV2D2 (nd_ntab_find, same nd_gtab_run) |
| CODEMODFX5 | Haiku | src/effects/dust.c (value call lines, include lines) | ready |
| CODEMODFX6 | Haiku | src/effects/sleep_bubble.c (value call lines, include lines) | ready |
| CODEMODFX7 | Haiku | src/effects/big_smoke_puff.c (value call lines, include lines) | ready |
| CODEMODFX8 | Haiku | src/effects/water_block.c (value call lines, include lines) | ready |
| CODEMODFX9 | Haiku | src/effects/embers.c (value call lines, include lines) | ready |
| CODEMODFX10 | Haiku | src/effects/shimmer_burst.c (value call lines, include lines) | ready |
| CONV2D | Haiku | 3ds/include/nd_gtab.h enum; 3ds/tools/nd_gfx_conv.c V2C struct, v2_walk top, new v2_unit_add, v2_emit_table; 3ds/tools/nd_gtab_test.c v2_compare | ready (rewritten 2026-11-13) |
| CONV2D2 | Haiku | nd_gtab.h NdNIdx and 2 prototypes; nd_gfx_conv.c v2_convert_all device index; nd_gtab.c after nd_gtab_find, gtr_cen; nd_walk.c nd_gtab_run :601 and :500; azt.d/bench.d CONV2D2.json | waits for CONV2D |
| CONV2F | Haiku | nd_gtab.h after the nd_rec_dl prototype; nd_gfx_conv.c main argc line, v2_convert_all signature and table loop; gen_gfx_tab.sh :18, :23; codemod.py argv loop, :204, :223; src/effects/water_splash.c:189 | waits for CONV2D2 |
| CODEMODW1 | Haiku | src/world/area_mac/mac_00/water_gfx.c value call lines, include line; new azt.d/bench.d CODEMODW1.json | ready |
| COVMAP | Haiku | 3ds/CMakeLists.txt inside if(PM_FNCOV) :386-391; new 3ds/tools/cov.d/README.txt, cov.d/COVMAP.txt, 3ds/tools/covsites.py | ready |
| GOLDFX1 | Haiku | new cov.d/azt.d/bench.d GOLDFX1 files | waits for COVMAP |
| GOLDENT1 | Haiku | new cov.d/azt.d/bench.d GOLDENT1 files | waits for COVMAP |
| GOLDWLD0 | Haiku | new cov.d/azt.d/bench.d GOLDWLD0 files | waits for COVMAP and the user's fortune scene |
