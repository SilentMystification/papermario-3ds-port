# Row KMR10M: kmr_10 walls show stretched green vertical lines on some frames while Mario moves

## User facts
User (2026-11-09, O3DS XL, build 92e013100): the kmr_10 wall garble is fixed (WALLT1). "While moving around that scene, sometimes
the wall texture will corrupt in a different way": "a bunch of stretched green vertical lines", on some frames, while moving.

## Start state
Worktree build3ds/wt/KMR10M, branch KMR10M from main. PM_ROW=KMR10M, PM_LANE=KMR10M. One Haiku agent.

## Cause (oracle, code trace, confidence about 65%)
- 3ds/src/nd_tex_set.c:304-309 tile_bound returns 1 when ens_src/ens_pal/fmt/siz/wrap/tile_sz match the last walker bind. It never
  checks that the pm_tex current slot (cur, pm_tex.c:107) still holds that image.
- Native paths call pm_tex_load (sets cur) without ens_clear: nd_tex_get (nd_tex.c:88), nd_tex_unit_hint miss (nd_tex.c:123), the
  native sprite path spr_tex_unit (pm_gpu.c about 3678). Mario's new walk frames and models coming into view do this: "while moving".
- The next cliff record (same main texture and window): ensure_tile(0) returns early (nd_tex_set.c:388), dual_bind skips its reload
  (nd_comb.c:634, same aux_key), and pm_tex_size (nd_comb.c:645) reads slots[cur] = the size of the last loaded texture.
- nd_comb.c:652-655 t1_p = tw0/tw1, th0/th1 get the wrong ratio -> unit 1 v scales outside the 64-row window -> clamped T repeats the
  edge row down each column (stretched vertical lines).
- New since WALLT1: td[1].src is now the LoadTile origin, so all cliff models share one aux_key and the reload path (with ens_clear)
  no longer runs between them.

## Fix steps (only 3ds/src/nd_tex_set.c)
1. Near the ens globals (about line 36): static int ens_slot = -1; static unsigned ens_gen;
2. ens_clear (line 302): also ens_slot = -1.
3. ensure_tile new-bind path, after pm_tex_load (about line 412): ens_slot = pm_tex_cur_idx(); ens_gen = pm_tex_slot_gen(ens_slot);
   Set ens_slot = -1 where the GAMEOVER rewin path (about 395) and the FB1 path (about 374) set ens_src.
4. tile_bound (304), after its first line: if (pm_tex_cur_idx() != ens_slot || pm_tex_slot_gen(ens_slot) != ens_gen) return 0;
   (pm_tex.h:15 and :66). Comment it KMR10M.
5. Debug counter (may stay): count the refusals of step 4; log "ensstale f<n> n <count>" once per 300 frames.
6. If the picture does not change: revert, then try the second fix point: in dual_bind compute tw0/th0 from td[0].w/h (pot, at least
   8) instead of pm_tex_size, and log tw0, th0 and t1_p once on the walk frame.

## Gates
- nd_host PASS.
- Byte-equal to a build-at of the base: spot_mac01_det, spot_mac00_det, battle_start_f182_cold, mdl5_iwa10_det, msgbox_talk_nowide,
  menu_open_det. A diff is allowed only on a frame with ensstale > 0 at a surface drawn after a native texture load. Else revert.
- New scenario kmr10_walk + bench kmr10_walk_pic (same commit): warp=kmr_10:0 pos=-142:136:221 story=ch0_toad_town
  padscript=go_-300_136_221,wait20,go_-60_136_221,wait20,go_-142_136_221 shots shotwin=150:12 loglevel=debug; until "shot 11 frame 161";
  cap 25 s. Pick waypoints that keep the low cliff in view. Run it on the base build first: stripes there confirm the cause.
  Pass: cliff in all 12 shots, no stretched lines, continuous strata, ensstale > 0 in the run.
- memcheck OK (about 8 bytes static). Console (user): walk in kmr_10 on the O3DS XL.
