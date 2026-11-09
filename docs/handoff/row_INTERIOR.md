# Row INTERIOR: room interiors (Toad House bed, Merlon's house) not drawn after the texture table fills

## User facts
User (2026-11-09, O3DS XL, build 92e013100): "Certain things aren't rendering properly, like the bed inside of the toad houses...
the insides of a lot of buildings aren't rendering properly." Earlier: Merlon's interior black on console, lit on Azahar (the MERLON
scenarios entered at about f88).

## Start state
Worktree build3ds/wt/INTERIOR, branch INTERIOR from main. PM_ROW=INTERIOR, PM_LANE=INTERIOR. One Haiku agent.

## Root cause (oracle, about 85%; Azahar run 151232 merlon_in_live_pic log #195)
- 3ds/src/nd_tex.c:47 `#define ND_TEX_N 128u` (256-slot index). Only reset: nd_tex_reset from pm_mem_log("map") (pm_mem.c:251) at a
  map load. nd_tex.c:88: when full, `pm_log_prof_line("ndtex full ...")` and return -1 (its comment "the caller falls back to the
  walker" is false since MDL-5e 856d74716).
- Sprites fill it: spr_tex_unit (pm_gpu.c about 3593-3600) gets a descriptor per raster/palette pair; mac_01 is full at about f124.
- Models: pm_gpu.c:3051-3052 model_draw `id = nd_tex_get(ds); if (id < 0) { nd_nat_why = 13; return -1; }` -> nd_recs.c:193 runs
  rec_dl only under ndmcheck/nomodelnat -> the model draws nothing, every frame, until the next map load.
- Interiors first request their textures at the door (late), under a black shroud: black room; models whose textures the exterior
  already used still draw ("certain things", the bed).
- Also affected: groups scripts show later, texture variation swaps, battle stages after a long walk (no reset at battle start),
  late water window descriptors (mac_00 pond), and every new sprite frame takes the executor fallback (CPU cost).

## Fix steps (nd_tex.c, 3ds/tools/nd_tex_test.c, one comment in pm_mem.c)
1. nd_tex.c:47-48: ND_TEX_N 1024u, ND_TEX_IDX 2048u (powers of two; tex_idx stays uint16_t).
2. `__attribute__((cold, noinline)) static void tex_flush(void) { nd_tex_reset(); tex_flushes++; }` plus `static unsigned
   tex_flushes;`. In nd_tex_get replace the full branch (:88) with
   `if (tex_n >= ND_TEX_N) { tex_flush(); h = desc_hash(&d) & (ND_TEX_IDX - 1u); }` then continue into the load path.
   Comment INTERIOR: a full table is flushed (tex_gen bumps, ids start again); since MDL-5e a refused model draws nothing.
3. Check (no code change, state it in the commit): no caller keeps an id across another nd_tex_get. model_draw uses its id in the
   same call; grep that nd_model_set.c does not call nd_tex_get; pm_gpu_model_hit gets its id on every hit; the sprite memo checks
   nd_tex_gen() (pm_gpu.c about :3596).
4. Replace the "ndtex full" line with `ndtex flush f%u n %u flushes %u` for the first 8 flushes, then every 64th.
5. Fix the pm_mem.c:251 comment; delete the stale "falls back to the walker" comment.
6. nd_tex_test.c: insert ND_TEX_N + 1 distinct descriptors; every id >= 0; nd_tex_gen() bumps by exactly 1; the last descriptor's
   words are its slot words; getting it again returns the same id with no new load.
7. Second fix point only if the late pictures do not change: run the late scenario with `census`, name the ndm why / ndmr class of
   the interior models, fix that code point. Two tries per named cause.

## Gates
1. nd_host PASS with the new test case.
2. New scenarios + benches (same commit), each run first on a build-at of the base (interior missing there proves the cause):
   - merlon_in_late_pic: merlon_in_live_pic with `wait240` before `go_-203_-280_100`, no det, shots inside the room; until the last
     shot frame; cap 25 s. Base log has `ndtex full` before `xfer door mac_01:out->mac_01:in`. Pass: room lit, Merlon visible,
     `ndtex flush` and no `ndtex full`.
   - toad_house_late_pic: `warp=mac_01:0 story=ch1_start`, outside the Toad House door (room collider 33, centre 442 -136, enterA
     426 -104), padscript wait240, walk to enterA, A, wait200, shots inside; cap 25 s. Pass: the bed and the room are drawn.
   - kmr20_in_late_pic: xfer_door_kmr20_shots with `wait600` before the door, shots inside. Pass: Mario's house beds drawn.
3. Byte-equal to a build-at of the base: spot_mac00_det, battle_start_f182_cold, mdl5_iwa10_det, msgbox_talk_nowide, nok11_det,
   xfer_door_kmr20_shots. spot_mac01_det may differ only as more models drawn (its frames are after the f124 fill).
4. memcheck: static grows about 46 KB (margin 1306 KB). If the late runs show few flushes, 512 entries (about 26 KB) is enough.
5. Console (user): walk in Toad Town 10 s or more, then enter the Toad House and Merlon's house: lit, bed drawn; `ndtex flush` lines.
