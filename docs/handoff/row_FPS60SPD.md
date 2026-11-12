# Row FPS60SPD: fps60 runs the game logic at up to 60 steps a second (2x speed)

Model: Haiku (one constant in one wait, one debug switch, one scenario copy; no design choice).
Base: main at `--commit 20d613506`. Worktree build3ds/wt/FPS60SPD, branch FPS60SPD. Goes ahead of every other open row.

## Cause (code read, oracle 2)
- 3ds/src/pm_main.c:657-668 (idle pass, fps60, no key frame drawn: `kn == 0`) waits only until
  `C3D_FrameCounter(0) - step_vb >= PM_VBLANKS_PER_STEP - 1` (1 vblank). The comment at :641-644 says the second vblank comes from
  `C3D_FrameBegin(SYNCDRAW)`. SYNC60 (52b70be00, pm_gpu.c:2751-2754) removed SYNCDRAW: the frame begin now waits only for the next
  59.83 Hz slot (sync60_pace, pm_gpu.c:2700-2711), one slot per frame begin.
- A step with a key frame has two frame begins (L in the step walk, K in pm_lg_show_keys, pm_main.c:637-639): 2 slots = 30 steps/s.
- A step with no key frame has one frame begin, and the idle wait gives 1 vblank: the next step starts 1 slot later = up to 60 steps/s.
- Paths that give a step with no key frame (every one goes through pm_main.c:661):
  1. LGF_ISKIP from INTERP1 `interp_heavy()` (pm_main.c:708, 0e4c3d718, merged 2026-11-11 18:35): heavy holds for 30 calm steps, so
     whole scenes run with ISKIP; nuGfxTaskStart (pm_nusys.c:182) then draws K at once and queues no key frame. This is the regression:
     builds pushed before INTERP1 (4393 and older) had ISKIP only on late steps.
  2. LGF_ISKIP on a late step (`iv > PM_VBLANKS_PER_STEP`, same line): the next step comes 1 slot early (old, rare).
  3. pm_interp_drop_keys at a mode change (pm_interp.c:90): one step.
  4. LGF_NOKEY (frame advance, pm_main.c:689): manual steps, no effect.
- Not a cause: FS1 skip (pm_main.c:683, 690-697) moves step_vb by 2 per skipped step; INTERPB1 and INTERPB2 do not change the cadence
  (twins and icmp only). INTERPB3: pm_key_show must call pm_lg_key_walked once per K frame (as pm_key_walk does); with this fix a
  missed call no longer speeds the game up.

## Edit 1 (3ds/src/pm_main.c, pm_frame_loop, the `else if (... pm_interp_on > 0)` idle wait at :657-668)
- `while ((int)(C3D_FrameCounter(0) - step_vb) < PM_VBLANKS_PER_STEP - 1 && guard--)` becomes
  `while ((int)(C3D_FrameCounter(0) - step_vb) < PM_VBLANKS_PER_STEP && guard--)`.
- Replace the comment at :641-644 with: "PC01, SYNC60: keep PM_VBLANKS_PER_STEP vblanks between game steps. A step with a key frame
  is paced by its two frame begins (sync60_pace) and does not come here. A step with no key frame (ISKIP, dropped keys) waits here
  for the whole step, because the frame begin waits only for one LCD period. A slow frame is already past it: no wait."
- No other edit to the schedule. Do not touch the PT88 time branch (:645-656) or line :708.

## Edit 2 (debug switch, same function, line :708)
- Condition becomes `(iv > PM_VBLANKS_PER_STEP || (pm_interp_on > 0 && (interp_heavy() || PMDL("iskipall"))))`. Debug only:
  every fps60 step skips the key frame, so the scenario below runs path 1 in Azahar. No doc edit.
- State (coder): f79344234 (Edits 1, 2), d8bddf45d (scenario), 14c057338. Build ok, memcheck static 21292 margin 694, nd_host PASS,
  spot_mac01_fps60_det 12/12 byte-equal. Try 1 of the scenario: SCENARIO BROKEN, no pace line: the copied `prof=4` turns logging
  off except prof lines (pm_log.c:655-675), and pace/pacewk are pm_log_d (pm_main.c:429, :433). Azahar ran at 7.5 fps, so no
  steps/s rule can be a gate. 2 of 8 Azahar starts used.

## Scenario fix (one commit, no code edit)
- `spot_mac01_fps60_iskip` debug string becomes exactly `warp=mac_01:0 pos=0:0:200 fps60 pacestat iskipall fs1off loglevel=debug` (no prof,
  no det, no shots; fs1off: an FS1 skip moves step_vb by 2 (pm_main.c:694), which can give iv 0 or 1 at the next drawn step). Until: `pacewk f([3-9]\d\d|\d{4,}) wk` (the first 5 s pace window that ends at frame 300 or later: a steady mac_01 window, no boot or warp in it).
  Cap 45, deadline 40. The bench entry stays as it is.

## Commands
- build: `3ds/tools/build.sh` (Git Bash) in build3ds/wt/FPS60SPD; nd_host: `python 3ds/tools/bench.py run nd_host --row FPS60SPD --why "nd_host"`.
- Azahar, one list: `python 3ds/tools/bench.py run spot_mac01_fps60_iskip,spot_mac01_fps60_det --row FPS60SPD --why "fps60 step rate, det bytes"`
  with PM_ROW=FPS60SPD PM_LANE=FPS60SPD. Byte compare: `bench.py base spot_mac01_fps60_det`, then
  `bench.py cmp spot_mac01_fps60_det --row FPS60SPD`.

## Gate (all three)
1. nd_host PASS.
2. spot_mac01_fps60_iskip (a code rule, not a speed): the `pace:` line printed just before the until line has the window count
   `intervals 1:0` (the per-window field, not `total 1:`). Expected with Edit 1: 0, because with iskipall every step goes through the
   idle wait (pm_main.c:657-668), which ends only at 2 vblanks after step_vb. Try 2 (591f4d57a, until on the first window f131) gave
   `total 1:4` in a window that holds the boot and the warp: boundary steps by code read, not a 2x path: pm_main.c:575 draws every
   pass until the first frame is drawn (`!lp_ever`), the first step after step_vb_ok is set has no ISKIP and shows a key frame
   (kn > 0 skips the idle wait, pm_main.c:639), and the warp load gives late steps. A 2x path would give 1-vblank intervals in most
   steps; that window had 4 of 130 and steps 112 for 301 vblanks (0.37 per vblank). Run only this scene once:
   `bench.py run spot_mac01_fps60_iskip --row FPS60SPD --why "fps60 iv rule, steady window"`. Hand back the pace and pacewk lines.
3. spot_mac01_fps60_det byte-equal to base (done: 12/12 at f79344234; the commits after it change only the scenario, no rerun).

## Stop
- Gate passes: commit (changed files only), delete this row file in the same commit, hand back the pace line and the cmp result.
- window intervals 1: above 0 in the steady window, cmp not equal, or a tool fault: stop, commit, hand back the exact report lines. Do not change the wait again.
