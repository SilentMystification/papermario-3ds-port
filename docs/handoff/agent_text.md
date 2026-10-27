# agent_text: repeated glyphs do not draw (see -> se, Boots -> Bo ts)

State: merged dlc-compiler-c1c2 (clean, no conflicts). No code change. No Azahar test run.

Suspect: PG1 skip in 3ds/src/pm_gbi.c G_SETTILESIZE (about line 3001). A repeated glyph with the same src, fmt, size, w, h, stride and epoch skips pm_gpu_flush and pm_tex_load. The skip sets tile_x0 = x0 - lp->ox and td[0] valid, and leaves cur and the GPU bind as they were. The first draw after a flush may not rebind unit 0. Not verified.

Next: in one shot, log the texture slot and tb_last at the first draw of the skipped glyph (the value the draw used). Check pm_tex_bind (3ds/src/pm_tex.c:963) on the new batch after a flush.

UPDATE (fix applied, UNTESTED): pm_gbi.c G_SETTILESIZE glyph path (pm_tex_glyph_same branch) set tile_x0 = x0 + ggx, while ensure_tile new-bind path uses x0 - gox. Changed the glyph path to x0 - ggx and y0 - ggy. Build blocked: generated headers missing in the worktree (vtx/stencil1.vtx.inc.c, effects/effect_defs.h). Copied ver/us/build from the main checkout; still not enough. No Azahar start made.

Coordinator update: the bug may also cover periods and spacing. Test shot must include a title/menu line with periods beside see and Boots. Report per glyph: buffer address, size, line stride, offset used by the draw.

TEST 1 (budget 1 of 8, no pass): build PM_LANE=text ok (generated headers copied from the main folder, git-ignored). Azahar run: `-Turbo -Smoke -FaultStop -Debug "fast shots shotwin=130:40 shotmode=1:1"`, result SMOKE FAILED timeout 25 s at map load (area 5 map 5). No shot of see or Boots, so no picture. Next: pick a scene that shows the title or menu text with see and Boots and periods, and log per bind SETTIMG and SETTILESIZE dims, stride, GPU dims, and the glyph buffer address, size and offset per glyph.

Do not flip UV signs. Do not restore the combiner-flush skip or texture-bind skip.
