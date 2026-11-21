# plan_upaway.md: star power lag (Up and Away, Star Beam, Peach Beam) (oracle UPAWAY; edit in place)
Tags: FACT (file:line, main 67b6f47608), GUESS, OPEN. User report 21 Nov (fact): the three sequences are very laggy on the O3DS.

## 0. Result
- The code read finds no single proven cause. It finds 4 candidate paths (section 2). Their costs are counts that no log has today.
- Row SPM0 (Haiku, ws_SPM0.md) adds a debug switch that starts any star power in a debug battle, and a census line `spm` per 30 steps.
- The SPM0 counts pick the fix row (section 4). No fix page is released before the counts.

## 1. Producers (FACT code read)
| move | script | per-step draw work |
|---|---|---|
| all three | StarPowerSupport.inc.c:47-66 shroud fade; :199-231 spirit NPC with WHITE_GLOW_BEHIND (npc.c:1959-1972: one energy_orb_wave) and NPC blur (npc.c:1147: 3 samples) | shroud is handled: MERLEE key mask pm_gpu.c:4001-4002 (shroud fog alpha out of the model key), BTLDIM GPU mix background.c:231-237 |
| Up and Away | up_and_away.c:83 SetMotionBlurParams 128, :93 back to 0 (fx_api.c:151-181 lerp over 10 steps) | OVERLAY_BLUR: screen_overlays.c:299-300 -> screen_render_util.c:36-77: 40 strips (39 of 6 rows + 1 of 5) per drawn frame, each a load tile of the old frame buffer and a texture rect |
| Up and Away | :86, :91 energy_shockwave x2, :89 shimmer_wave (11 parts), :157 sparkles x10 (5 steps apart), :112 star icon virtual entity | small: 1 to 11 draws each |
| Star Beam, Peach Beam | StarBeamSupport.inc.c:166-168 fx_peach_star_beam: 7 (Star) or 8 (Peach) spirit sprites + beam (peach_star_beam.c:174-258) | about 10 draws |
| Star Beam, Peach Beam | StarBeamSupport.inc.c:115-129, :131-148: each spirit spawns fx_misc_particles(4, ..., 10 particles, 20 steps) every 21 steps | 7 to 8 instances live x 10 particles = 70 to 80 particle draws per step (GUESS: arithmetic, not counted). Each particle: own matrix, prim, tile size, list call (misc_particles.c:314-353). misc_particles is not on the NdParts run (nd_rec.c:516; users: shimmer_burst.c, pink_sparkles.c, snowfall.c, embers.c, effect_3D.c, snaking_static.c, water_block.c) |

## 2. Candidates, ranked (GUESS order; the deciding counter in the spm line)
| # | path | file:line | moves | deciding counter |
|---|---|---|---|---|
| 1 | effect draw volume: about 80 extra small draws per step. Console cost per draw: core 0 207 us, core 1 232 us (FACT plan_critpath.md section 4) | misc_particles.c:322-352 | beams | `gd` (PICA draws) and `u` (core 1 units) per step, beam windows against the k0 window |
| 2 | old-frame blur: 1 GPU copy at the frame begin (pm_gpu.c:3137, :2800), 1 off-screen pass with the grab quad (pm_gpu.c:2942-3002), 40 rects in one batch (nd_tex_set.c:306-333; with pm_wout: :357-373) | screen_render_util.c:68-76 | Up and Away | `t3` (old-frame grabs), `cap` (CPU frame reads), `spl`, `gd` |
| 3 | CPU frame read fallback: 76800 VRAM texel reads on core 0 (pm_gpu.c:2305-2320) when the GPU copy is not there: first frame of a request, a T2 grab earlier in the frame (pm_gpu.c:2960) | pm_main.c:349-389 | Up and Away | `cap` per window above 1 |
| 4 | first-sight loads: effect code and graphics read from ROM (effects.c:299-384, freed 3 steps after the last instance :101-117), the move overlay (use_star_powers.c:120), spirit sprites | effects.c:335, :376 | all | `rr`, `rkb`, `rs`, `tm`, `tkb`, `fn` per window |
- The blur stays on after Up and Away: fx_api.c:177 sets progress 0, render_screen_overlay_backUI (screen_overlays.c:444-450) draws while progress is not -1, and only a battle start resets it (btl_states_actions.c:226). FACT. The `post` windows of spm_uaa show this cost. A skip of a 0-alpha overlay is a draw cut: user decision (no culling rule), not a page.

## 3. Measurement row
- SPM0 (Haiku): page ws_SPM0.md. Scenarios spm_uaa (spmove=7), spm_sbeam (8), spm_pbeam (9). Counts are Azahar facts. Milliseconds need the console.
- Console capture after SPM0 merges (orchestrator asks the user first). One push runs all three moves in one battle (order 8, 9, 7: Up and Away last, its blur stays on):
  capture_check: `python 3ds/tools/capture_check.py --row SPM1 --profile prof --proof "spmove: start,spm k1,spmove: done" warp=kmr_20:0 battle=BTL_KMR_1_FORMATION_00:BTL_KMR_1_STAGE_01 battlef=30 spmove=8:9:7 fixedseed profdisk=1`
  push: `powershell -NoProfile -File 3ds/go.ps1 push o -ip 192.168.1.170 -NoBuild -Seconds 150 -Profile prof -GameDebug "warp=kmr_20:0 battle=BTL_KMR_1_FORMATION_00:BTL_KMR_1_STAGE_01 battlef=30 spmove=8:9:7 fixedseed profdisk=1"`
  Read: the spm lines per window, the prof `ft`, `cls`, `stepx max` lines of the windows inside each move. OPEN: the battle needs 3 player turns (Mario HP 10, one Goomba: GUESS it holds).

## 4. Decision rules (after SPM0; each a closed page)
- `gd` in the beam windows minus k0 `gd` per step is 40 or more: page PARTB (Sonnet, reason: a new particle run form). misc_particles draws go through nd_parts_begin/add/end as shimmer_burst.c does. Gate miscp_drec byte-equal plus one beam drec window.
- `cap` above 1 in an Up and Away window: one Azahar run with the word `caplog` (pm_main.c:340-345, field `why`) names the path; then page CAPFIX (Haiku).
- `t3` is 1 per step and `gd` rises by 3 or less: the blur costs one copy and 2 draws per frame. No page. The console `cls` decides.
- `rr` or `rs` above 0 in a window after k1: page FXROM (design pass first; MTX rows own the ROM read drain, plan_maptrans.md).
- None of the above: the extra time is GPU fill (OPEN, needs C3D_GetDrawingTime per frame; a census row, not a fix).
