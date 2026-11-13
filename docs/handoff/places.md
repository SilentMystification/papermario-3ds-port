# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Plan v3 (plan_endgame.md) is accepted (user, 2026-11-12). FPS60FL is merged (d2b2b3b2a).

| row | model | places | state |
|---|---|---|---|
| HOSTDIR | Haiku | 3ds/tools/nd_host_test.sh one new line after `/tmp/kcc` (:73); new 3ds/tests/host.d/README.txt, 00_hostd.sh | ready (first: the 5 rows below wait for it) |
| DEADDEL1 | Haiku | src/draw_img_util.c :216-343, :359-530; src/background_gfx.c :324-344; src/effect_utils.c :59-111; src/model.c comment block to :4897 | ready |
| FSELLAG2 | Sonnet (needs the user's yes) | 3ds/src/pm_gpu.c pm_gpu_off_begin signature and clear, its callers :2656 :2667 :2685; pm_gpu.h; nd_target.c off_screen_check | waits for FSELLAG1 |
| BOTHUD | Haiku | 3ds/src/pm_log.c sink_main wait line (:888); 3ds/src/pm_gpu_ui.c pm_gpu_hud_flush counter; 3ds/src/pm_main.c probe next to shotbottom (:813-818); new azt.d/bench.d BOTHUD.json | ready |
| CODEMODFX1 | Haiku | src/effects/water_splash.c, src/effects/music_note.c (value call lines, one include line each) | ready |
| N64DEL2 | Haiku | 3ds/tools/unifdef3ds.py condition regex, 3ds/tools/ppcmp.sh gcc line; TARGET_3DS lines of include, src/world, src/entity, src/battle, src/pause, src/filemenu, src/audio, src/evt, src/os | ready |
| N64DEL3 | Haiku | TARGET_3DS lines of src/*.c (root only) | waits for N64DEL2 |
| NPSO3 | Haiku | new 3ds/tools/gen_pso_tab.py, gen_pso_main.c, nd_psotab_test.c, 3ds/tests/pso/, 3ds/tests/host.d/psotab.sh; 3ds/build.sh new gen_pso_tab after gen_gfx_tab (:100-101) and key_now sha1sum list; .gitignore one line | ready |
| CONV2C | Sonnet (accepted) | 3ds/tools/nd_gfx_conv.c, gen_gfx_tab.sh, nd_gtab_test.c; new generated 3ds/src/nd_ntab_descs.c; .gitignore one line | ready (NPSO3 numbers its descs: merge NPSO3 first or CONV2C waits for it at its last step) |
| BOOTSTATE | Haiku | 3ds/src/pm_warp.c one block at the end of pm_warp_tick; new azt.d/bench.d BOOTSTATE.json | ready (evidence for the unstable bases) |
| HOSTSLOT | Haiku | 3ds/build.sh slot block (:153-159) and `gen_map_restore` line (:47), docker run lines :75 :92 :224, the --parallel line :242 and the BUILD FAILED line; 3ds/tools/nd_host_test.sh top and docker lines :15 :81; new 3ds/tests/buildlock/hostslot.sh | ready (tool fault first) |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py | not started (user decides) |
