# Row DOORHITCH: the hitch at a house door (room open/close in the same map)

## User facts
User (2026-11-09): "entering and exiting a house still causes a noticeable hitch. Can this be prevented?" O3DS XL kmr_20 xfer lines:
door in 75.7 ms max (mdl 507 misses, tex 17/326 KB/51.3 ms first entry), out 67.6 (mdl 444), in again 73.5 (mdl 420, tex 36 KB 4.4 ms).

## Start state
Worktree build3ds/wt/DOORHITCH, branch DOORHITCH from main (hash in the first commit). PM_ROW=DOORHITCH, PM_LANE=DOORHITCH. One Opus agent.

## Facts (oracle)
- xfer fields: pm_xfer.c:49-55, 126-139; fade == max for doors (no load/init step). mdl counts key misses only (nd_recs.c:123); a key
  hit refused by view mask / mver (pm_gpu.c:3119-3120, ndn_rng) rebuilds uncounted.
- A door: SetDoorState (rooms.c:230); interior groups g20/g21/g49/g56 enabled at ENTER_BEGIN (first draw: dlc, tex, misses);
  InterpCamTargetPos 10 frames; RoomVisibilityToggleImpl (rooms.c ~277) ShroudTintAmt +32 per step; the first toggle switches every
  model outside g62 to ENV_TINT_SHROUD (model.c:1487): record w0 render mode 10, R3 flags +2/16/32, fog/blend/fog pos (model.c:1901-1990):
  every outside model misses in that step (MERLEE mask pm_gpu.c:2817 covers only alpha/blend). Store rule (nd_model.c:577-590) keeps
  both entries; mc_trim removes every entry unused for 30 frames (nd_model.c:520-560; NDMC_N 1024, trim at 768, pool 512 draws), so the
  other room state is gone by the next door: whole-state rebuild (about 0.12-0.15 ms per miss on the O3DS: 420-507 misses = 55-75 ms).
- ROM reads per door: player rasters (101b90_len_8f0.c:353) and action overlays (7E9D0.c:284); each drains the walk (pm_os.c:433).
- Sizes: mc_p5 176 KB (352 B per draw, only use5 draws need it), mc_pool 144 KB, mc_keyw 368 KB, mc_tab 64 KB.

## Plan
1. Measure: PmXf gains mdl_rng (nd_recs.c), rdw_t (drain ticks from nuPiReadRom while pm_xf.on), max-step snapshot (mdl, rng, tex,
   dlc, rom, smp = prof sample step); cold line `xfer3 <kind> n<seq> | maxstep mdl a rng b tex ms dlc ms rom n smp 0/1 | rng tot trim n
   phi n ehi n rdw ms`; cold debug line in CreateMapRoom (door centre, posEnterA/B). Scenario+bench xfer_door_kmr20 (Azahar, det,
   xferdiag, census; warp=kmr_20:0, walk to the door, A, wait, exit, twice; cap 45 s; until `xfer2 door n4\b`), console bench
   xfer_door_kmr20_prof (prof=30), an _shots variant over the shroud ramp. Decide H1 (trimmed other state), H2 (key word churn, xferk
   names it), H3 (view-mask rebuilds), H4 (prof sample step).
2a (H1): trim first removes entries with a stale handle or dlc generation (key words NDMC_RW+25/26), then oldest-uf only down to 5/8;
   mc_p5 becomes a side pool for use5 draws only (about 128 x 352 B), the saving funds mc_pool 512 -> about 896 draws; static about
   -20 KB; a full p5 pool trims instead of no-store. Expect warm door mdl < 50, max < 40 ms.
2b (H2): mask/patch the named word like MERLEE (pm_gpu.c:2817, 3141-3145). 2c (H3): re-cut ranges on a view-mask change, reuse state.
3. First entry: at map load, under the covered MAPX-3 warm frames (pm_xfer.c:110), compile dlc of hidden models and load their
   textures while free slots exist (no eviction of visible ones).
4. ROM drain (only if rdw > 10 ms per door): drain only when the raster cache target was used by the frame being walked.
hw_guidelines; no toggles; no stubs; memcheck margin >= 256 KB (print it).

## Done criteria
nd_host PASS; byte gates spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, xfer_door_kmr20_shots
byte-equal to build-at base; ndmcheck diff 0 (spot_mac01 and a kmr_20 door); xfer_map_shots covered frames still black; tour10_det
10 xfer lines. Azahar direction: xfer_door_kmr20 max, mdl, rng, trim before/after. Console (user): xfer_door_kmr20_prof and
xfer_door_prof: max < 50 ms, smp 0 at the max step.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No git stash, no push, no attribution. Never commit 3ds/src/map_restore_table.c. DIRECT edits pm_gpu.c and
nd_* in parallel; KEYMISS (later) covers key churn: merge main before each step.

## At the end
Delete this row file in the completing commit. SendMessage main "DOORHITCH ready <hash>".
