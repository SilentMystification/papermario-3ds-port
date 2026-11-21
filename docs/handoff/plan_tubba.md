# plan_tubba: Tubba Blubba's attack cloud does not fade (it goes black)

Oracle TUBBA. User report (fact, 21 Nov): the attack cloud does not fade when Tubba Blubba hits Mario. It fades to black, not to transparent.
Pages: ws_TUB0 (census and bench), ws_TUB1 (shockwave, green_impact, red_impact), ws_TUB2 (5 other effects). All Haiku.

## 1. Producer (FACT code read)
- Attack: src/battle/area/arn/actor/tubba_blubba.c:474-479 (hit, Label 0) plays EFFECT_SHOCKWAVE 0 and EFFECT_SMOKE_IMPACT. :502 plays SHOCKWAVE 1.
  dgb and dig have the same lines (dgb :599-645, dig :443-484).
- shockwave_main remaps arg 0 to type 3 and arg 1 to type 2 (src/effects/shockwave.c:32-44). Type 3: prim (220,210,200), env (255,255,250),
  life 60 steps (:78, :108-113). The fade: `alpha *= 0.94` each step from step 8 (:177-180). The setup list of type 2 and 3 is D_09000A80:
  colour lerp(PRIM, ENV, T0 x (1 - T1)), alpha PRIMa x T0a, G_RM_ZB_CLD_SURF2 (blend on).
- Draw (shockwave.c:322-354): the part loop is recorded once into a stream sub range (nd_sub_open :322, nd_sub_close :344). Then pass 1 sets
  prim/env >> 2 (dark), CULL_FRONT and calls the range (:345-349). Pass 2 sets the full prim/env, CULL_BACK and calls it again (:350-354).
  The per-step fade value is primA, written at :345 and :350 only.
- smoke_impact (src/effects/smoke_impact.c): fade = flip book (tile 0 and tile 1 windows) and env alpha. All values are set before each
  list draw, outside any sub. Its list snap takes them. No fault found on this path. Textures of both effects are I4/IA4 = GPU_LA4
  (pm_tex.c:416, :419). The DEC1 key fill runs only for GPU_RGBA5551 (pm_tex.c:1636): not on this path.

## 2. Cause (FACT code read)
- Core 1 builds the snap of a draw from nd_ds at record time (nd_snap.c snap_build0: in.prim, in.env, the plan colours by pm_comb_colours_k).
  The list snap table of nd_draw_list is built while the list records (nd_gtab.c nd_rec_ntab: nd_lsc_begin, nd_lsc_end_at).
- nd_sub_call records NDC_CALL and replays only the unit log into nd_ds (nd_rec.c nd_rec_call, nd_ds.c nd_ds_range). Core 0 runs the
  range as recorded (nd_rec.c stream_run case NDC_CALL), so the draws apply the snaps of nd_sub_open time (nd_snap.c nd_snapchk_tri: nd_comb_apply,
  ls_gpu prim/env). The TEV stage constant is the plan constant of that snap (nd_tmpl.c:206).
- Result: the prim, env and cull set between nd_sub_close and nd_sub_call never reach the TEV. The shockwave keeps the colours and the alpha of
  the last draw that core 1 recorded before shockwave.c:322. The fade (primA) is lost. Both passes draw with the same colours.
- The black colour: the stale prim/env come from an earlier draw (OPEN which one: TUB0 prints the values). A black env (for example
  smoke_impact env 0,0,0) gives colour lerp(PRIM, black, T0) = a dark cloud (GUESS).
- 60 fps: the K frame copies the L words (pm_gpu.c:178). The wrong colour is in L. No K row.
- Same fault class (state set after nd_sub_close, before nd_sub_call): green_impact.c:208-218, red_impact.c:215-225, hieroglyphs.c:176-178,
  stop_watch.c:254-255, thunderbolt_ring.c:137-148, firework.c:303-313, whirlwind.c:257-270 (tile sizes and colours). Not affected:
  confetti, shimmer_burst, shimmer_wave (only matrices between the calls; matrices are core 0 records, nd_xf.c xf_rec).

## 3. Fix rule
- A sub range must not depend on state set after nd_sub_open. Shape A: one state for all calls: move the state calls above nd_sub_open.
  Shape B: other state per call: record the body once per call (a static body function). Shape C: a sub called once: record it inline.
- No core 0 change, no recorder change. Core 1 records 2 to 10 more list draws per effect step (shape B) (GUESS: small).

## 4. Rows
| row | model | content | gate |
|---|---|---|---|
| TUB0 | Haiku | subst census (nd_rec.c, nd_cmd.h), fxsp marker (pm_fxtest.c), subcol_drec scenario and bench | drec byte-equal; subst at f412 slot; none in the control slot |
| TUB1 | Haiku | shape B in shockwave.c, green_impact.c, red_impact.c | subst 0 in slots 0-2; subcol_drec expected-diff fxsp; fxparts_drec byte-equal |
| TUB2 | Haiku | shape B hieroglyphs.c, stop_watch.c; shape A thunderbolt_ring.c, firework.c; shapes A+C whirlwind.c | subst 0 in all slots; same benches |
- TUB1 and TUB2 start after the TUB0 merge (the bench must be on main). They edit disjoint files and can run in parallel.
- Counters C1..C12: none (OPEN: this oracle did not read the list; a colour fault moves no list counter).
- subcol_drec slots (fxtest order, slot = 44 frames from frame 400, drec frame = slot start + 12): 0 shockwave 32 (f412), 1 green_impact 44 (f456),
  2 red_impact 98 (f500), 3 hieroglyphs 106 (f544), 4 stop_watch 92 (f588), 5 thunderbolt_ring 82 (f632), 6 whirlwind 97 (f676),
  7 firework 61 (f720), 8 shimmer_burst 50 (f764, control). Indices FACT (pm_fxtest.c FXLIST order). Alive at k 12: GUESS.
