# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| HOSTDIR | Haiku | 3ds/tools/nd_host_test.sh one new line after `/tmp/kcc` (:73); new 3ds/tests/host.d/README.txt, 00_hostd.sh | ready (first: the 5 rows below wait for it) |
| NPSO2 | Sonnet | new 3ds/src/nd_decide.c, 3ds/tools/nd_decide_test.c, 3ds/tests/host.d/decide.sh; nd.h one struct after NdTevOut; nd_comb.c nd_comb_decide and the code it moves; nd_tmpl.c only if needed | waits for NPSO1 |
| DEADDEL1 | Haiku | src/draw_img_util.c :216-343, :359-530; src/background_gfx.c :324-344; src/effect_utils.c :59-111; src/model.c comment block to :4897 | ready |
| FSELLAG1 | Haiku | 3ds/src/pm_gpu.c pm_gpu_off_begin :2134, pm_gpu_off_end last line (:2427) | ready |
| FSELLAG2 | Sonnet (needs the user's yes) | 3ds/src/pm_gpu.c pm_gpu_off_begin signature and clear, its callers :2656 :2667 :2685; pm_gpu.h; nd_target.c off_screen_check | waits for FSELLAG1 |
| AMBWARP | Haiku | 3ds/src/pm_warp.c warp_end_intro (the stop lines and its log), pm_warp_world_mode (start); new azt.d/bench.d AMBWARP.json | ready |
| BOTHUD | Haiku | 3ds/src/pm_log.c sink_main wait line (:888); 3ds/src/pm_gpu_ui.c pm_gpu_hud_flush counter; 3ds/src/pm_main.c probe next to shotbottom (:813-818); new azt.d/bench.d BOTHUD.json | ready |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
