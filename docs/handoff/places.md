# Places of open rows (orchestrator edits this file in place; a row with an equal or overlapping place waits)

One line per open row: row | model | file: function or region names | merge state. Delete the line when the row is merged or closed.
A shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json) is never a place of a row: see HANDOFF.md
"Parallel work without merge conflicts". Open rows at the handoff of 2026-11-12 (second update): none runs. TOOLFIX, REGDIR1-3, FPS60W2, S1TSTEN, S7ENT, S7EFX7 and S7EFX8 are merged. The next row is the freeze fix of the oracle read: it edits 3ds/src/pm_gpu.c pm_gpu_key_show :2946-2955 (fills and VRAM copies queued again) and the stream walk end pm_gpu_end_w; FPS60VAL and FRAMEHASH edit nearby lines of the same file and wait for it.

| row | model | places | state |
|---|---|---|---|
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py; scenario kcdump_mac01 | waits for the next session (user decides after the console test) |
| LISTCEN2 | Haiku | 3ds/tools/listcen.py; bench_defs.json listcen entry | waits for the plan |
| ELFGATE | Haiku | new 3ds/tools/elfgate.py, elfgate_patterns.txt | waits for the plan |
| N64DEL1 / N64DEL2 / N64DEL3 | Haiku | new 3ds/tools/unifdef3ds.py (N64DEL1); TARGET_3DS conditional lines of src/effects / src/world, src/entity, src/battle, src/pause, src/filemenu, include / src/*.c | waits for the plan, LISTCEN2 |
| NPSO | Opus | 3ds/include/nd_api.h; new 3ds/src/nd_decide.c; nd_comb.c and nd_tmpl.c split; new gen_pso_tab.sh; build.sh one generator step (after REGDIR3); nd_host_test.sh; nd_direct.c psochk hook; src/draw_img_util.c | waits for the plan, user Opus yes |
| CONV2 | Opus | 3ds/include/nd_gtab.h; 3ds/tools/nd_gfx_conv.c, nd_gtab_test.c; new 3ds/src/nd_ntab.c; nd_rec.c nd_draw_list; 6 pilot effect files | waits for NPSO |
| NXF | Sonnet | 3ds/include/nd_api.h (3 calls, after NPSO); new 3ds/src/nd_xf.c; src/effects/ring_blast.c, stat_change.c | waits for NPSO |
| FRAMEHASH | Sonnet | 3ds/src/pm_gpu.c frame end (fh_frame) and :2805 kc_rec condition (conflicts with FPS60VAL's kc_build end only if both edit the same lines: run after FPS60W/FPS60VAL merge); bench.py azahar_hash kind; scenario files | on hold (user) |
