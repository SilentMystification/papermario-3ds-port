# Row HC7: rect path fast paths (plan_hotcode.md HC7, class G) - wave 3

## User facts
User (2026-11-09): shrink the hot code a lot; queue the stages in waves with no merge conflicts. HC0: class G M1 21.7 KB, M3 on mac_01
12.1 KB. Shared oracle plan (code read; the HC0 fncov counts were lost with the HC0 lane, so step 1 measures again).

## Console fact (2026-11-09, XL main 8085ef1d0, mac_01 standing still)
Windows with 33 text rects per frame: nd 23.1 ms, I-miss 55k; with 5-15 rects: nd 17.9-20.2 ms, I-miss 40-44k. About 20 extra text
rects cost about 5 ms and about 15k I-misses per frame (about 250 us per rect). 136 rects: nd 32.7 ms, 91k I-miss. User: start now
(before HC4 ends). Gate add: a rect census (rects per frame and us per rect) before/after on a menu or HUD scene.

## Start state
`sh 3ds/tools/wt_new.sh HC7` now (user, 2026-11-09), while HC4 runs: skip step 2 (ensure_tile, nd_tex_set.c is HC4's file) and step 7 (HC7b) for now. PM_ROW=HC7, PM_LANE=HC7. One Haiku agent.
Files: 3ds/src/nd_rect.c, pm_gpu_rect.c, nd_tex_set.c (ensure_tile, tile_bound), pm_tex.c (cell_get only); HC7b adds nd_cmd.h,
nd_rec.c, 3ds/tools/nd_cmd_test.c. Not with HC8 at the same time if both edit pm_gpu.c.

## Steps (each a commit, byte-equal)
1. fncov_mac01_det and fncov_kmr10_det on the base (PM_FNCOV=1 PM_LANE=HC7): record class G call counts; `bench.py base` for both gates.
2. ensure_tile (nd_tex_set.c:354): `if (td[n].valid && tile_bound(n) && !fb_cand(n)) { fx/fy; cur_tile = n; return; }` with
   fb_cand(n) = `!pm_wide && fmt == 0 && siz == 2 && stride == 320`; the rest becomes ensure_tile_slow (cold, unchanged). tile_bound:
   the KMR10M stale log (:310-314) goes cold, the refusal stays inline.
3. nd_texrect (nd_rect.c:159-265): inline decode, one rare-flag test -> nd_texrect_rare (cold: depth-copy query, rect_to_image,
   texlog, zcol, box pattern, bgclamp log, G_ZS_PRIM, off-screen mirror fold); COPY cycle -> nd_texrect_copy (noinline); else the fast
   compute and pm_gpu_tex_rect. box_pattern_rect body cold with the inline guard `box_pat && td[0].valid && td[1].valid`.
4. pm_gpu_tex_rect / tex_raw_rect (pm_gpu_rect.c:222, :249): the one-piece path inline; the wide edge stretch, the split loop and
   zmforce -> tex_raw_split (cold).
5. cell_get (pm_tex.c:277): cell allocation, the eviction scan, cell_write and the full-cells log -> cold noinline.
6. nd_fillrect (nd_rect.c:267): CPUFILL, depth fill and G_ZS_PRIM arms -> nd_fillrect_rare (cold).
7. HC7b only after HC4c merged (recorder tile/image/othermode shadow): new NDC_FILLRECT (nd_cmd.h value 10) and a widened
   NDC_TEXRECT with resolved fields (corners with the cycle edge rule, s0/t0 with tile shift, dsdx/dtdy, descriptor id, fold class,
   colour-image role, box_pat); nd_cmd_test.c for both formats.

## Gates (light)
- nd_host PASS. Byte-equal against stored base shots: msgbox_talk_nowide, pause_spin_grab_det_w300.
- Size: M1 class G <= 5 KB; M3 class G on mac_01 <= 5 KB; nd_texrect fast path <= 1 KB. memcheck OK.
