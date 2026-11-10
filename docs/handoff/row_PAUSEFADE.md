# Row PAUSEFADE: the wide side strips of the pause screen do not darken with the centre

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh PAUSEFADE`; PM_ROW=PM_LANE=PAUSEFADE on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## User facts
Pause with wide on. The centre darkens and the 40-px side strips stay at full brightness. Scenario pause_gut_dbg_w300.
## Cause (code)
The centre is PM_CC_43 = (PRIM - TEXEL0) * PRIM_ALPHA + TEXEL0 with prim (40,40,40, gPauseBackgroundFade), fade 0..128 (src/background_gfx.c:384-408). That is lerp(texel, grey 40, fade/255).
The strips: pm_gpu_pause_gutters (3ds/src/pm_gpu.c:2193) draws class 2 (TEXEL0 x SHADE) with white vertices and no fade. nd_target.c:63 drops the fade arg that gPmNdGutter carries (background_gfx.c:441).
The fix: the V06 mix stage (pm_gpu_set_mix: "mixes the colour to rgb of rgba by its alpha", gs.r_mix plus r_mixcol, pm_gpu.c:214) gives lerp(colour, rgb, a) exactly.
## Premise and its check
Premise: raw rect batches honour gs.r_mix and r_mixcol. Check first: the TEV key of the raw batch (pm_gpu.c near :708-718, tk.mixcol = r_mixcol) reads gs.r_mix for the raw path. If it does not, hand back at once.
## Touch only
3ds/src/pm_gpu.c (pm_gpu_pause_gutters), 3ds/src/nd_target.c (:63). pm_gpu.c is also in rows STEREOFX-1 and STEREOFX-2: run them one at a time.
## Exact edits
1. nd_target.c:63: call `pm_gpu_pause_gutters(on)`. Change the extern to `void pm_gpu_pause_gutters(unsigned fade);`.
2. pm_gpu.c: `void pm_gpu_pause_gutters(unsigned fade)`. After `gut_tex = t;` add
   `const int s_mix = gs.r_mix; const unsigned s_mixc = r_mixcol; gs.r_mix = 1; r_mixcol = 0x28282800u | (fade & 0xffu); /* PAUSEFADE: lerp to (40,40,40) by the fade, as PM_CC_43 */`
   After `flush();` add `gs.r_mix = s_mix; r_mixcol = s_mixc;`.
   Update the comment at nd_target.c:63 ("no arg") to say the arg is the fade.
## Done criteria
1. Build OK. nd_host PASS.
2. `bench.py run pause_gut_dbg_w300 --row PAUSEFADE --why "strip fade"`. In the last shot (P6, 15-byte header "P6 240 400 255\n"), compare the mean RGB of rows 36-39 with rows 40-43, and rows 356-359 with rows 360-363 (a short py -3 -I script). Pass: each pair within 8 levels per channel. Also run the same script on the base shot (`bench.py base pause_gut_dbg_w300`): the pairs differ there.
3. Byte gate: `bench.py cmp pause_spin_grab_det_w300 --row PAUSEFADE`. Only the strip pixels may change.
Stop: 2 tries, then hand back with the 4 means.
