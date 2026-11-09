# Row SHROUD: sprites take the stale background dim of the last model (user: star spirits dark, Up and Away dim wrong, 2026-11-09)

## User facts
XL: "Up and Away star spirit power doesn't properly dim the background." "Lighting on the star spirits is wrong, they are dark during
the animation." User approved the fix (touches every sprite draw).

## Cause (shared oracle, code read)
Star powers darken with ENV_TINT_SHROUD (StarPowerSupport.inc.c:47-66; up_and_away.c:14-31). On the N64 only models and the
background are shrouded; sprites (actors, the spirit NPC_BTL_SPIRIT, StarPowerSupport.inc.c:205-212) stay bright. The port's shroud
is the walker blend mix (nd_set.c nd_mix1 :187-193, set by sync_render :207-218 into gs.r_mix, r_mixcol). The native sprite path copies
gs.r_mix / r_mixcol straight into its state (pm_gpu.c pm_gpu_spr_build, about :3700) without a sync for its own othermode, so a sprite
inherits the last shrouded model's mix. Since EDGEORD sprites draw right after the shrouded opaque models. Expected r_mix 0 for a
TEX_EDGE sprite; the port has 1. The background-dim part is a weaker inference (same stale/global mix decides which non-record draws
get the shroud).

## Start state
`sh 3ds/tools/wt_new.sh SHROUD`, branch SHROUD. PM_ROW=SHROUD, PM_LANE=SHROUD. One coder agent. Scenario btldim_star_pic (exists).

## Steps
1. Log check (debug word sprmix, cold): per sprite build during a star power: gs.r_mix, r_mixcol, nd_ws.omode_l >> 30. Expect r_mix 1.
2. Fix: in pm_gpu_spr_build (and nd_ent native draws if they copy gs.r_mix the same way) take the mix from the draw's own othermode
   (`int m = nd_mix1(nd_ws.omode_l, &col)`, r_mix 2 for the FOG_SHADE_A case as in sync_render), or call sync_render() first.
3. Gates: nd_host PASS; btldim_star_pic: actors and spirits bright, stage dark (compare to base); a star power shot with the spirits;
   spot_mac01_det and battle_start_f182_cold byte-equal (no shroud there); memcheck OK.
4. Commit; delete this row file in the completing commit. Budget: 5 Azahar starts.

## Touch only
3ds/src/pm_gpu.c (pm_gpu_spr_build), nd_ent.c (if needed), azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
