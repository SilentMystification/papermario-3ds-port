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
| ENTRYCAP | Haiku | 3ds/src/nd_gtab.c nd_rec_dl :84-85; 3ds/src/nd_walk.c entcap_rec (added before nd_gtab_run), first line of the non-state-only arm of nd_gtab_run; new 3ds/tools/entcap.py, azt.d/bench.d ENTRYCAP.json | running (CONV2D2 merged 0f5d3960b); runs with CONV2F |
| CONV2F | Haiku | nd_gtab.h after the nd_rec_dl prototype; nd_gfx_conv.c main argc and call lines, v2_convert_all signature, a write after its table loop; gen_gfx_tab.sh :18, :23; codemod.py argv loop, :204, :223, docstring Mapping; src/effects/water_splash.c:189 | ready (lint 0 at 0f5d3960b); runs with ENTRYCAP |
| CODEMODW1 | Haiku | src/world/area_mac/mac_00/water_gfx.c value call lines, include line; new azt.d/bench.d CODEMODW1.json | parked (CODEMODW1 trace) |
| GOLDWLD0 | Haiku | new cov.d/azt.d/bench.d GOLDWLD0 files | waits for COVMAP and the user's fortune scene |
| N64DEL4 | Haiku | the 9 skip blocks: include/PR/os_convert.h, common.h, common_structs.h, include_asm.h, macros.h, src/entity.c, kzn2 lava_piranha.c, WaterEffects.inc.c, lava_piranha/part1.inc.c | waits for N64DEL2 and N64DEL3 |
| CODEMODW2 | Haiku | src/world/area_iwa/iwa_01/splash.c value call lines, include lines; new azt.d/bench.d CODEMODW2.json | parked (CODEMODW1 trace) |
| CODEMODW3 | Haiku | src/world/area_flo/flo_10/waterfx.c value call lines, include lines; new azt.d/bench.d CODEMODW3.json | parked (CODEMODW1 trace) |
| CODEMODW4 | Haiku | src/world/area_hos/hos_04/fountains.c value call lines, include lines; new azt.d/bench.d CODEMODW4.json | parked (CODEMODW1 trace) |
| SEEDPROBE1 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its 12 lines; new azt.d/bench.d SEEDPROBE1.json | waits for SEEDTOOL (pilot) |
| SEEDPROBE2 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE2.json | waits for SEEDPROBE1 merge (adjacent lines of one file) |
| SEEDPROBE3 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE3.json | waits for SEEDPROBE2 merge (adjacent lines of one file) |
| SEEDPROBE4 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE4.json | waits for SEEDPROBE3 merge (adjacent lines of one file) |
| SEEDPROBE5 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE5.json | waits for SEEDPROBE4 merge (adjacent lines of one file) |
| SEEDPROBE6 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE6.json | waits for SEEDPROBE5 merge (adjacent lines of one file) |
| CODEMODB1 | Haiku | the value call and include lines of effect_3D, snowfall, pink_sparkles, radiating_energy_orb, quizmo_stage, peach_star_beam, whirlwind, landing_dust, damage_indicator (src/effects); new azt.d/bench.d CODEMODB1.json | waits for the live verdicts of its 9 seeds (SEEDPROBE1, 3, 4, 6); pilot batch |
