# Row KZN04: kzn_04 runs at 15-16 fps on the O3DS XL (user, 2026-11-09)

(Model: Haiku. A diagnostic row: 3 Azahar starts.)

## User facts
"The room kzn_04 is very laggy." "Perhaps it's because of the fireballs." "Fire sticks."
XL log (main de4f4b8c8, build3ds/hw_live_log_131.txt 2975-3006): fps 15.7/16.6, cls tex 23.4/23.7 ms (mac_01 under 2), ph tex 6.8, ndc trect 5.4 per frame.

## Cause (oracle, code read, main fe241dc42)
kzn_04 calls SetSpriteShading(SHADING_KZN_04) (src/world/area_kzn/kzn_04/main.c:15). Every NPC sprite (12 fire bar NPCs, ANIM_Fire_Brighest_Burn,
npc.c:115-263), Mario and the partner draw shaded. Per shaded sprite per frame: create_shading_palette (src/sprite_shading.c:66) draws the 16x1
palette rect (the trect count), then set_tilesize tile 0 -> tex_shaded0 (3ds/src/nd_tex_set.c:542) -> pm_gpu_flush + load_shaded_sprite
(nd_tex_set.c:620: 256 float cc_cycle1) -> pm_tex_load_shaded (3ds/src/pm_tex.c:1802). That function keys the texture by a hash of the 256-entry
lut, and the lut changes when a sprite moves through the lights (the fire bars rotate every frame): a miss, decode and upload per sprite per frame.
Premise: most pm_tex_load_shaded calls in kzn_04 are misses, about 3 or more per frame.

## Goal
Print the shaded-load count, miss count and time per 300 frames. The oracle designs the fix from these numbers. No fix in this row.

## Start state
`sh 3ds/tools/wt_new.sh KZN04` (build3ds/wt/KZN04, branch KZN04 from main). PM_ROW=KZN04, PM_LANE=KZN04.

## Touch only
3ds/src/pm_tex.c (pm_tex_load_shaded only), 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json. NOT nd_tex_set.c (row HC7).

## Exact edit (pm_tex.c, at pm_tex_load_shaded, line 1802)
Above the function add:
  static unsigned shs_calls, shs_miss, shs_t, shs_f; /* KZN04 diag (switch shadestat): shaded sprite loads since the last line */
  static __attribute__((cold, noinline)) void shs_note(unsigned t0, int miss) {
      shs_calls++; shs_miss += miss != 0; shs_t += pm_ticks() - t0;
      const unsigned f = pm_frame_count();
      if (f >= shs_f + 300u) { pm_log("shadestat f%u frames %u calls %u miss %u us %u", f, f - shs_f, shs_calls, shs_miss, shs_t / 268u); shs_f = f; shs_calls = shs_miss = shs_t = 0; }
  }
In the function: first line `const int shs_on = PMDL("shadestat"); const unsigned t0 = shs_on ? pm_ticks() : 0u;`.
Replace `return load_impl(...);` with `int r = load_impl(...); if (__builtin_expect(shs_on, 0)) shs_note(t0, ph_miss); return r;` (same arguments).

## Scenario and bench (same commit)
Scenario kzn04_shadestat: debug "warp=kzn_04:0 shadestat loglevel=debug", until "shadestat f(6|7)\\d\\d ", cap 30, deadline 20,
first_marker "warp: kzn_04 entry 0", needs_picture false, expected_events warp + until. Bench kzn04_shadestat (azahar_stream, same scenario, cap 30).

## Done criteria
1. Build OK, memcheck line OK.
2. `python 3ds/tools/bench.py run kzn04_shadestat --row KZN04 --lane KZN04 --why "KZN04 shaded load counts"`: the line
   `shadestat f6xx frames 3xx calls N miss M us U` prints. Report N, M, U, and N/300 calls per frame.
   Premise true: M/N >= 0.9 and N/300 >= 3. Premise false: report the values (the cost is elsewhere).
3. Commit. Give the push recipe for the XL: same debug word plus prof, scene kzn_04. Azahar gives only the counts.
Affected scenarios: kzn04_shadestat only.

## Stop
3 Azahar starts. 2 tries for a missing line, then hand back with the log tail. STOP RULE: 90 min or 150 tool calls.

## Rules
CLAUDE.md rules 1-7. PM_LANE=KZN04 on every build.sh and bench call. No git stash. Foreground waits.

## Commands
- Build: PM_ROW=KZN04 PM_LANE=KZN04 sh 3ds/build.sh
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget KZN04

## Fix design (oracle batch 2)
Cause chain (nd_tex_set.c:542-660, pm_tex.c:1809):
- Each shaded sprite builds a 256-entry lut on the CPU: cc_cycle1 of 16 sprite-palette colours x 16 shading-palette colours.
- The shading palette is the 16x1 rect of PM_CC_55, lerp(ENV highlight, PRIM shadow, tile 2 texel alpha) (sprite_shading.c:385-396). It moves with the fire-bar lights every frame.
- The key = FNV(lut, dx4, dy4), so a moving light is a miss.
- A miss: expand_dual of the whole w x h (pm_tex.c:496), plus a memset of the scratch, plus a new slot and an upload.
The true fix is GPU-side, with no misses. Cycle 1 is PM_CC_50 = TEXEL0 x TEXEL1 with alpha TEXEL1, and TEXEL0 = lerp(env, prim, a_j). So: unit 1 = the sprite with its palette (static); unit 0 = an L8 map of a_j for the sprite's indices (static), sampled at the offset; TEV = lerp of 2 constants by tex0, then x tex1. That is a design row (new TEV class, 2 units, nd_tex_set.c, so after HC7 and COINSTAR). Recommend Sonnet: a new class and template, not mechanical. Its picture is not byte-equal (the N64 palette is 5-bit), so it needs a user OK.
What 14 calls per frame do not explain (console tex 23.5 ms vs 3.6 ms on Azahar):
- 14 CPU texrects per frame write SpriteShadingPalette; each write can run pm_tex_invalidate over the slots.
- 14 pm_gpu_flush batch closes (tex_shaded0).
- Slot churn from 9 new slots per frame (evict_oldest_done).
CPU fix row: row_KZN04F.md. GPU design row: Sonnet recommended, needs a user OK (picture not byte-equal), after HC7 and COINSTAR.

## At the end
The debug line stays. Keep this file (the fix row follows). Hand back "KZN04 ready <hash> calls N miss M us U".
