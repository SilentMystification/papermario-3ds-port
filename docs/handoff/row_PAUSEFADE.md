# Row PAUSEFADE: the wide side strips of the pause screen do not darken with the centre

(Model: Haiku. Branch PAUSEFADE at 123ddcc46, worktree build3ds/wt/PAUSEFADE. PM_ROW=PM_LANE=PAUSEFADE on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep it. Commit after each passed step. Delete this row file in the completing commit.)

## User facts
Pause with wide on. The centre darkens. The 40 px side strips stay at full brightness. Scenario pause_gut_dbg_w300.
## Cause (code)
- The centre is PM_CC_43 = lerp(texel, grey 40, fade/255), prim (40,40,40,gPauseBackgroundFade), fade 0..128 (src/background_gfx.c:384-408).
- The strips: pm_gpu_pause_gutters (3ds/src/pm_gpu.c:2196) draws class 2 (TEXEL0 x SHADE), white vertices.
- The try at 123ddcc46 set gs.r_mix. nd_tmpl.c:217 builds the mix stage only for `draw_3d && !rect_2d`. The gutter batch is rect_2d, so no pixel changed (40 of 40 shots byte-equal).
- The fix: use rect class 6. nd_tmpl.c:166-174 (tex && combine 6) builds INTERP(CONST, TEX0, CONST alpha) = prim * prim_a + texel * (1 - prim_a), alpha = texel alpha. With prim = (40,40,40,fade), that is exactly PM_CC_43. This path does not check rect_2d. prim is in the TEV key for combine 6 (pm_gpu.c:412, :709).
## Keep from 123ddcc46
nd_walk.c:337 (`a & 0xffu`) and nd_target.c:63 (passes the fade). Do not change them.
## Touch only
3ds/src/pm_gpu.c, function pm_gpu_pause_gutters. No other file.
## Exact edits (pm_gpu.c, pm_gpu_pause_gutters)
1. Delete the r_mix line: `const int s_mix = gs.r_mix; const unsigned s_mixc = r_mixcol; gs.r_mix = 1; r_mixcol = ...;`
2. Delete `gs.r_mix = s_mix; r_mixcol = s_mixc;` after the last flush().
3. Keep the `flush();` after `gut_tex = t;`. After it, add:
   `const unsigned s_prim = prim; prim = 0x28282800u | (fade & 0xffu); /* PAUSEFADE: rect class 6 = prim * prim_a + texel * (1 - prim_a) (nd_tmpl.c:166), prim (40,40,40,fade): PM_CC_43 as the centre */`
4. Change `const int ec = 2;` to `const int ec = 6;`. Change its comment to: `/* PAUSEFADE: class 6, the texel faded to prim by prim alpha */`.
5. After the last `flush();` add `prim = s_prim;`.
6. Change the comment above the function ("class 2 with a white vertex colour ... open, row PAUSEFADE") to: `/* PAUSEFADE: the gutter batch is rect class 6 with prim (40,40,40,fade): the strips fade as the centre (PM_CC_43). */`
## Done criteria
1. Build OK. memcheck OK.
2. `bench.py run pause_gut_dbg_w300 --row PAUSEFADE --why "strip fade class 6"`. In the last shot (P6, 15-byte header "P6 240 400 255\n"; a row is a screen column), compare the mean RGB of rows 36-39 with rows 40-43, and of rows 356-359 with rows 360-363 (short `py -3 -I` script). Pass: each pair within 8 levels per channel. The base shot (`bench.py base pause_gut_dbg_w300`) has a large difference in each pair.
3. Byte gate: `bench.py base pause_spin_grab_det_w300`, then `bench.py cmp pause_spin_grab_det_w300 --row PAUSEFADE`. Only rows 0-39 and 360-399 may change.
Stop: no change in the strip means: hand back with the 4 means. Do not try a third route.
