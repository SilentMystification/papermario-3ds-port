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
| CODEMODW1 | Haiku | 3ds/src/nd_val.c (comment, includes, val_rec, nd_val_run); 3ds/include/nd_val.h (comment, nd_val_run prototype); src/world/area_mac/mac_00/water_gfx.c (done at fc6cbc44f); azt.d/bench.d CODEMODW1.json | ready (resume on a2c877bf5, correction 2) |
| GOLDWLD0 | Haiku | new cov.d/azt.d/bench.d GOLDWLD0 files | waits for COVMAP and the user's fortune scene |
| N64DEL4 | Haiku | the 9 skip blocks: include/PR/os_convert.h, common.h, common_structs.h, include_asm.h, macros.h, src/entity.c, kzn2 lava_piranha.c, WaterEffects.inc.c, lava_piranha/part1.inc.c | waits for N64DEL2 and N64DEL3 |
| CODEMODW2 | Haiku | src/world/area_iwa/iwa_01/splash.c value call lines, include lines; new azt.d/bench.d CODEMODW2.json | waits for the CODEMODW1 merge |
| CODEMODW3 | Haiku | src/world/area_flo/flo_10/waterfx.c value call lines, include lines; new azt.d/bench.d CODEMODW3.json | waits for the CODEMODW1 merge |
| CODEMODW4 | Haiku | src/world/area_hos/hos_04/fountains.c value call lines, include lines; new azt.d/bench.d CODEMODW4.json | waits for the CODEMODW1 merge |
| SEEDPROBE1 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its 12 lines; new azt.d/bench.d SEEDPROBE1.json | waits for SEEDTOOL (pilot) |
| SEEDPROBE2 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE2.json | waits for SEEDPROBE1 merge (adjacent lines of one file) |
| SEEDPROBE3 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE3.json | waits for SEEDPROBE2 merge (adjacent lines of one file) |
| SEEDPROBE4 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE4.json | waits for SEEDPROBE3 merge (adjacent lines of one file) |
| SEEDPROBE5 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE5.json | waits for SEEDPROBE4 merge (adjacent lines of one file) |
| SEEDPROBE6 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE6.json | waits for SEEDPROBE5 merge (adjacent lines of one file) |
| CODEMODB1 | Haiku | the value call and include lines of effect_3D, snowfall, pink_sparkles, radiating_energy_orb, quizmo_stage, peach_star_beam, whirlwind, landing_dust, damage_indicator (src/effects); new azt.d/bench.d CODEMODB1.json | waits for the live verdicts of its 9 seeds (SEEDPROBE1, 3, 4, 6); pilot batch |
| CODEMODB2 | Haiku | the value call and include lines of aura, balloon, big_snowflakes, blast, bombette_breaking, butterflies, chomp_drop, cold_breath, confetti, debuff (src/effects); new azt.d/bench.d CODEMODB2.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB3 | Haiku | the value call and include lines of drop_leaves, effect_46, effect_65, effect_75, effect_86, ending_decals, energy_in_out, energy_shockwave, falling_leaves, fire_flower (src/effects); new azt.d/bench.d CODEMODB3.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB4 | Haiku | the value call and include lines of flashing_box_shockwave, floating_cloud_puff, floating_flower, floating_rock, fright_jar, gather_energy_pink, gather_magic, green_impact, hieroglyphs, huff_puff_breath (src/effects); new azt.d/bench.d CODEMODB4.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB5 | Haiku | the value call and include lines of ice_pillar, ice_shard, light_rays, merlin_house_stars, moving_cloud, purple_ring, quizmo_assistant, quizmo_audience, red_impact, ring_blast (src/effects); new azt.d/bench.d CODEMODB5.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB6 | Haiku | the value call and include lines of shimmer_wave, shiny_flare, small_gold_sparkle, smoke_burst, smoke_impact, snowflake, snowman_doll, sparkles, squirt (src/effects); new azt.d/bench.d CODEMODB6.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB7 | Haiku | the value call and include lines of stars_shimmer, stars_spread, static_status, steam_burst, stop_watch, sweat, throw_spiny, thunderbolt_ring, tubba_heart_attack (src/effects); new azt.d/bench.d CODEMODB7.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB8 | Haiku | the value call and include lines of waterfall, energy_orb_wave, fire_breath, flame, radial_shimmer, shape_spell, smoke_ring, star_outline, water_fountain (src/effects); new azt.d/bench.d CODEMODB8.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB9 | Haiku | the value call and include lines of windy_leaves, attack_result_text, breaking_junk, cloud_puff, cloud_trail, damage_stars, disable_x, effect_63, emote (src/effects); new azt.d/bench.d CODEMODB9.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB10 | Haiku | the value call and include lines of firework, got_item_outline, lightning, lightning_bolt, misc_particles, recover, rising_bubble, shattering_stones, shockwave (src/effects); new azt.d/bench.d CODEMODB10.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| CODEMODB11 | Haiku | the value call and include lines of something_rotating, spiky_white_aura, spirit_card, star_spirits_energy, stars_burst, stars_orbiting, stat_change, sun, walking_dust (src/effects); new azt.d/bench.d CODEMODB11.json | waits for the live verdicts of its seeds (rowlint needs faults 0) and the CODEMODB1 pilot |
| SEEDPROBE7 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE7.json | waits for SEEDPROBE6 merge (adjacent lines of one file) |
| SEEDPROBE8 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE8.json | waits for SEEDPROBE7 merge (adjacent lines of one file) |
| SEEDPROBE9 | Haiku | docs/handoff/row_SEEDS.md verdict fields of its lines; new azt.d/bench.d SEEDPROBE9.json | waits for SEEDPROBE8 merge (adjacent lines of one file) |
