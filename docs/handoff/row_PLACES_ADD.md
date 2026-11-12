# Places of the rows written by ORACLE7 (for docs/handoff/places.md; the oracle writes only row files: copy these lines, then delete this file)

| row | model | places | state |
|---|---|---|---|
| TOOLFIX | Haiku | 3ds/push_3ds.sh :169-177 (stale check, SEND FILE line); 3ds/go.ps1 push verb block, Push-One call; 3ds/tests/push/run.sh case (b), new case (b2) | ready |
| REGDIR1 | Haiku | 3ds/tools/gen_gfx_tab.sh :7-19; new 3ds/tools/gconv_src.d/ | ready |
| REGDIR2 | Haiku | 3ds/tools/azt.ps1 :44, :124, :451-470; 3ds/tools/bench.py :88-89, :208-223; new 3ds/tools/azt.d/, bench.d/; test_catalog.md one paragraph | ready |
| REGDIR3 | Haiku | 3ds/CMakeLists.txt :343-430 (PORT_SRC); 3ds/build.sh :68, :86 | ready |
| FPS60W | Haiku | 3ds/src/pm_gpu.c :1655, :1842, :2461, :2479, :2789, :2941 and a new function above pm_gx_room; 3ds/src/pm_wdog.c :218-222; one scenario wdhang_mac01 (azt.d file after REGDIR2, else after spot_mac01_det) | ready |
| FPS60VAL | Sonnet | 3ds/src/pm_gpu.c end of kc_build, end of pm_gpu_key_show; new 3ds/tools/kcval.py; scenario kcdump_mac01 | ready (may run with FPS60W: other lines) |
| LISTCEN2 | Haiku | 3ds/tools/listcen.py; bench_defs.json listcen entry | waits for the plan |
| ELFGATE | Haiku | new 3ds/tools/elfgate.py, elfgate_patterns.txt | waits for the plan |
| N64DEL1 / N64DEL2 / N64DEL3 | Haiku | new 3ds/tools/unifdef3ds.py (N64DEL1); TARGET_3DS conditional lines of src/effects / src/world, src/entity, src/battle, src/pause, src/filemenu, include / src/*.c | waits for the plan, LISTCEN2 |
| NPSO | Opus | 3ds/include/nd_api.h; new 3ds/src/nd_decide.c; nd_comb.c and nd_tmpl.c split; new gen_pso_tab.sh; build.sh one generator step (after REGDIR3); nd_host_test.sh; nd_direct.c psochk hook; src/draw_img_util.c | waits for the plan, user Opus yes |
| CONV2 | Opus | 3ds/include/nd_gtab.h; 3ds/tools/nd_gfx_conv.c, nd_gtab_test.c; new 3ds/src/nd_ntab.c; nd_rec.c nd_draw_list; 6 pilot effect files | waits for NPSO |
| NXF | Sonnet | 3ds/include/nd_api.h (3 calls, after NPSO); new 3ds/src/nd_xf.c; src/effects/ring_blast.c, stat_change.c | waits for NPSO |
| FRAMEHASH | Sonnet | 3ds/src/pm_gpu.c frame end (fh_frame) and :2805 kc_rec condition (conflicts with FPS60VAL's kc_build end only if both edit the same lines: run after FPS60W/FPS60VAL merge); bench.py azahar_hash kind; scenario files | on hold (user) |
Shared registries: after REGDIR1/REGDIR2 merge, rows add gconv_src.d/<row>.txt and azt.d/<row>.json, bench.d/<row>.json only.
The waiting rows S7EFX7 and S7EFX8 edit gen_gfx_tab.sh lines 15-16: merge them before REGDIR1, or move their names into
gconv_src.d/S7EFX7.txt and S7EFX8.txt at their merge.
