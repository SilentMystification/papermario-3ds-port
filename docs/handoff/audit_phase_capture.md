# Audit: phase capture for Toad Town pricing

Scope: read only. Last log: build3ds/hw_live_log_170.txt (build 1587, git aa54485e5, loglevel 3, switches phase fsplit pmu).
No source edits, no runs.

## (1) os_report and G_PM_MODEL (0x0c)

- Opcode: `include/gbi_custom.h:218` G_PM_MODEL 0x0C. G_PM_TEX 0x0A, G_PM_TEXI 0x0B, G_PM_RM 0x09.
- `os_class_of` (`3ds/src/pm_gbi.c:1970-1983`): 0x0C has no case, so it falls to OS_OTH. G_PM_RM (0x09) also falls to OTH.
- Sample: `os_op_hook` (`pm_gbi.c:1995-2004`) runs before the walker switch (`pm_gbi.c:3585`). A sample on 0x0C
  covers the whole `mg_run` call (`pm_gbi.c:3472-3525`) up to the next hook.
- Nested words run in `mg_run` do not call `os_op_hook`. They are not in `os_op_n`, `os_nn` or `tot_n`.
- Result: the premise of a double count is not right. 0x0C time is counted once, in class `oth`. Nested ops are
  missing from the op counts, and their time sits in 0x0C.
- Log evidence (f1799, `hw_live_log_170.txt` line 751): `oth 22156/161`, `top 0c:18568`. Frame f2099 (line 908): `oth 25835`, `0c:19815`.
  The class sum does not double count. Excluding 0x0C would remove 19 of 26 ms from the breakdown.
- Smallest fix (count, one line): in `mg_run` after the skip check (`pm_gbi.c:3479`), add `if (pm_ph_on) os_nn[op]++;`.
  `tot_n` (`pm_gbi.c:2284`) then counts nested ops. Class time stays unchanged.
- Smallest fix (per-op time, one token): add `|| pm_ph_on` to the gate at `pm_gbi.c:3473`. The walker then runs the
  group words and the op hook times them. Cost: a phase run does not use the MR1 fast path. Use this for pricing.

## (2) Position and window lines

- No log line prints camera or player position on each window.
- `camlog` (`pm_warp.c:619-633`, switch `camlog`) prints eye, target and player position, max 60 lines per second. It is not in the push switches.
- `pm_pos_text` (`pm_route.c:86-90`) returns Mario and camera positions, but only for the on-screen HUD (`pm_main.c:513`).
- Window counter: `opsplit f<n>` and the other `f<n>` lines use `rm_frame` (`pm_gbi.c:2394`). A window is 300 drawn frames.
- Smallest line: in `os_report` (`pm_gbi.c`, after line 2306) add `extern void pm_pos_text(char (*)[40]);`, a `char p[2][40]` buffer,
  and `pm_log_hw("pos f%d %s | %s", rm_frame, p[0], p[1]);`. One line per 300 frames, no per-frame cost. Still versus walk is
  the change of the M x y z values between two consecutive `pos` lines.

## (3) Scene gating

- Arrival marker exists: `load map area 1 map 2 scan 2960` (`pm_map.c:572`, `pm_log_d`, needs loglevel=debug). Log line 644.
- Phase windows do not reset on map load. `os_fr` (`pm_gbi.c:2331`) and the counters are cleared only in `os_report` (`pm_gbi.c:2316-2318`).
  `pm_ph_scene_map` (`pm_phase.c:62`) only stores the map ids. `fsplit` (`pm_phase.c:211-222`) does not reset on load either.
- Log evidence: window `f1499` (line 667) includes the file select (`ui:` line 628, `f1443`) and the Toad load. Window `f1799` (line 751) is the first clean window.
- Smallest fix: `pm_gbi.c` exports `void pm_gbi_win_reset(void)`, which sets `os_fr = 0` and runs the memsets at `pm_gbi.c:2316-2318`.
  Call it from `pm_ph_scene_map` (`pm_phase.c:62`). The `fsplit` counters need the same reset (`pm_phase.c:199-222`).
  Then only windows after the last map load print.

## (4) Phase cost

- Tick reads: `pm_ticks` (`pm_main.c:157`) increments `pm_svc_n`. `opsplit` field `pt` is the svc reads per frame (`pm_gbi.c:2303-2304`).
  Log: `pt 596` to `pt 1015` per frame in the Toad windows (lines 667 to 1219).
- No phase-off reference exists in one run. `fsplit` runs without phase (`pm_phase.c:287`, `pm_ph_frame_end` calls `fs_frame_end` before the `pm_ph_on` return at line 477).
- No new code. For a phase-off gbi reference, run once with `fsplit` only, same scene, same window.

## (5) Lines in the next log (phase fsplit pmu loglevel=debug, Toad Town)

Valid for pricing (after the reset in (3), phase inflates absolute times, shares are valid):
- `build` (`pm_main.c:627`): build id.
- `load map area 1 map 2` (`pm_map.c:572`): scene start marker.
- `ui:` (`pm_phase.c:407`): menu events, marks only.
- `fsplit f` (`pm_phase.c:213`): frame split.
- `opsplit f` (`pm_gbi.c:2304`): class shares. Use `oth` with the 0c term noted. Use `top` as ranking.
- `prod f sum` and `prod f <name>` (`pm_gbi.c:2306, 2313`): producer shares.
- `mops f` (`pm_gbi.c:2065`): redundant state op counts.
- `mdlsum f` (`pm_gbi.c:2240, 2251`): model draw counts.
- `mr0 f` (`pm_gbi.c:2266`): per plain model us split (valid from f1799, 92 plain models per frame).
- `ms0 f` (`pm_gbi.c:2271`): opaque runs and closes.
- `dlc2 f` (`pm_dlc.inc.c:532`): dlc counts, present in the log.
- `tx1 f` (`pm_tex.c:768`): texture misses, phase gate.

Not valid for pricing the Toad walk:
- `pmu f` (`pm_phase.c:260`): hardware counters, only the `pmu` switch. Optional.
- `pcopen`, `pcphase`, `pcgbi` (`pm_phase.c:443-453`): pause open only.
- `dcsum` (`pm_gpu.c:2214`), `camlog` (`pm_warp.c:628`), `poshud`: not in the push.

Lines before the reset (f299, f599, f899, f1199, and f1499) mix title, file select and map 5 5. Do not price from them.
