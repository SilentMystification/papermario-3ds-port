# Row INTERP1: fps60 interpolation only when two walks fit a step (O3DS frame time grows with fps60 on)

(Model: Haiku. Bug row: user fact, O3DS: with frame interpolation the frame time increases instead of decreasing.)

## Cause (oracle code read, main 6e171b39a)
With pm_interp_on (switch `fps60` or Options row 0, pm_interp.c:83-87; switch `nointerp` turns it off) every drawn step is walked
twice on core 0: nuGfxTaskStart (pm_nusys.c:164-166) pushes the frame with PMF_REPL (the t = 0.5 pass: pm_frame_walk walks every task,
load_mtx looks each matrix up in pm_interp_find, nd_mtx_set.c:92), and pm_step_run (pm_main.c:496, :502) queues the key frame:
pm_key_walk (pm_nusys.c:191-200) walks every task of the step again (gbi_task_w -> nd_stream_run). Same stream, same draw core, same
I-cache misses, a full GPU frame each; no frame buffer copy. Core 1 adds pm_interp_lerp (pm_interp.c:126) per step. So core 0 needs
2 x walk per 33.4 ms step: heavy scenes walk 8-18 ms (xl4a), 16-36 ms of walk per step plus the GPU tail waits. The only fallback
is late: LGF_ISKIP (pm_main.c:690) after a step took more than 2 vblanks; the next step walks twice again (late, one walk, late).
sync60_pace (pm_gpu.c:2459) paces each frame begin at 59.83 Hz; the idle wait is the vblank schedule (pm_main.c:639-650).

## Fix
Decide the interpolation per step from the measured walk: core 0 times its walks (2 tick reads per task); a step interpolates only while
the last step's walk is under 13 ms (two walks fit with the GPU tail); over it, ISKIP (one walk per step, as fps60 off) until 30
steps in a row walk under 10 ms. The same limits on the GPU drawing time of the last frame (C3D_GetDrawingTime). A census report of
the decision per 300 steps.

## Start
`sh 3ds/tools/wt_new.sh INTERP1`. PM_ROW=INTERP1 PM_LANE=INTERP1 on every build and bench call.

## Touch only
3ds/src/pm_nusys.c (gbi_walk, pm_frame_walk, pm_key_walk, new globals), 3ds/src/pm_main.c (the LGF_ISKIP line :690, a new static
function above pm_frame_loop).

## Exact edits
1. pm_nusys.c: `unsigned pm_iw_us[3]; /* INTERP1: walk us of the last t = 0.5 pass [0], key frame [1], plain frame [2] */` and
   `static unsigned long long iw_tk;`. gbi_walk: `const uint64_t w0 = svcGetSystemTick();` before nd_stream_run and
   `iw_tk += svcGetSystemTick() - w0;` after it (keep the pm_ph_on lines). pm_frame_walk: `iw_tk = 0;` first; at its end
   `pm_iw_us[(f->flags & PMF_REPL) ? 0 : 2] = (unsigned)(iw_tk / 268u);`. pm_key_walk: `iw_tk = 0;` before its task loop and
   `pm_iw_us[1] = (unsigned)(iw_tk / 268u);` after it.
2. pm_main.c above pm_frame_loop: `extern unsigned pm_iw_us[3];` and `static int interp_heavy(void)`: static int heavy, calm; static
   unsigned n, nh, sl, sk, sp, sg; `unsigned w = heavy ? pm_iw_us[2] : (pm_iw_us[0] > pm_iw_us[1] ? pm_iw_us[0] : pm_iw_us[1]);`
   `unsigned g = (unsigned)(C3D_GetDrawingTime() * 1000.f);` `if (w > 13000u || g > 13000u) { heavy = 1; calm = 0; } else if (heavy &&
   w < 10000u && g < 10000u) { if (++calm >= 30) heavy = 0; } else calm = 0;` then n++, nh += heavy, sl += pm_iw_us[0], sk +=
   pm_iw_us[1], sp += pm_iw_us[2], sg += g; when n == 300: `pm_log_hw("interp f%u steps %u heavy %u lerp_us %u key_us %u plain_us %u
   gpu_us %u", pm_frame_count(), n, nh, sl / n, sk / n, sp / n, sg / n);` and set all six to 0. Return heavy.
3. pm_main.c:690: `if (!pm_det_on() && (iv > PM_VBLANKS_PER_STEP || (pm_interp_on > 0 && interp_heavy()))) lc.flags |= LGF_ISKIP;`

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK; nd_host PASS.
- `bench.py base spot_mac01_fps60_det`; `bench.py run spot_mac01_fps60_det --row INTERP1 --why "INTERP1: det never sets ISKIP"`;
  `bench.py cmp spot_mac01_fps60_det --row INTERP1 --commit $(git merge-base HEAD main)`: 6/6 byte-equal. Budget: 2 Azahar starts.
- Console: none in this row. The batch console run with fps60 on (Options row 0) carries the interp census; the user checks the feel.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output. Delete this row file in the completing commit.
