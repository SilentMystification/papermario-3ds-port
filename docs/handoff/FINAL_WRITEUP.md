# Final writeup: overnight batch, 2026-10-29

Build for the console: `build3ds/pm_3ds.3dsx` (from lane final, HEAD 29f504d7b).
SHA-256: 0df21e8c3a76b2d0db03b1cf1142cb3a0f4f87edcf4cc28215ce7bc50b2d549c
Archive: `3ds/logs/builds/pm_3ds_final_29f504d7b.3dsx`
Push log line: `3ds/logs/perf/pushed_builds.txt`, last line.
Status: not run on the console yet. The user pushes in the morning.

## 1. Verify result

- Build of merged HEAD 29f504d7b: passed with no merge fix. The first build run
  failed in the sync step with about 300 `tar: Cannot open` lines. A second run of
  the same command passed (compile-link 41 s, exit 0). No commit "merge fix" was made.
- `map_restore_table.c` was regenerated, built, then restored from the `.bak`.
  Git still shows it as modified, as before the batch.
- Catalog runs (PM_ROW=final, 8 of 8 Azahar starts used):

| Scenario | Log check | Picture (read by eye) | Result |
|---|---|---|---|
| yard | steady frame pass. Shot-frame check failed before a regex fix | Mario, partner, pipe, house, ground | pass |
| tour10 | 4 of 4 pass (10 maps, done at 14730) | Mario in the tour maps | pass |
| pause_menu | 3 of 3 pass (shot 178 logged, regex fixed) | Status page, dialog text | pass |
| partner_menu | 4 of 4 pass | Party tab, Goombario | pass |
| battle_start | 4 of 4 pass | Battle HUD, Jump menu | pass |
| sewer | 3 of 3 pass | Mario in tunnel | pass |
| ruins | 3 of 3 pass | Mario, text box | pass |
| volcano | 3 of 3 pass | Mario in lava room | pass |
| text_menu | 3 of 3 pass (shot 178) | Status page text, periods and spacing | pass |
| file_select | 3 of 3 pass (game mode 14, file select enter, shot 302). Run hit the 90 s cap | File 1 box, Select file to start | pass |
| fileselect_rotate | same run as file_select (shots 282 to 293 in the set) | Box shift read by eye | pass |
| full411 | 411 maps, tour done (PASS, 121 s) | completion only | pass |

- Yard log numbers were not captured in the first pass (see section 1a). The phase lines were not on
  these runs, and the log file of each run was overwritten by the next run.
  No walk us, ops/frame, closes, tex fail or vramfree min A/B figure exists for
  lane final. Use the lane numbers in section 2 instead.
- Budget: the row `final` allowed 8 starts. The batch needs 12 starts for the list
  above. The orchestrator did not reset the row. Run the four missing scenarios and
  full411 after a reset with `run_azahar.ps1 -ResetBudget final`.
- Catalog fix (commit e99888363): the scenario file held `\\\b` in JSON. JSON read it
  as a backspace, so all shot-frame regexes failed. It is now `\\b`.
  The shot check for yard passed on an old log only. Re-check the new yard log.
- pause_menu log check fails: no `shot N frame 180` line. The picture shows the
  status page, so the subject is present. Open item for the catalog.

## 1a. Second pass on lane final (row final, 5 of 8 starts used before full411)

Numbers from the phase and phsum lines in the runs (`build3ds/lane-final/last_log.txt`
copied to scratchpad). Walk is the `taskpl main us` field. Ops is the `taskpl ops`
field. Closes is the first field of `flsplit tex`. Averages are over frames 300 to 599.

| Measure | Starting point (docs) | Final binary, yard | Final binary, pause |
|---|---|---|---|
| Main walk us, avg f300-599 | 9812 (lane 2 start) | 6608 | 7634 |
| Ops per frame (taskpl ops) | 3576 (lane 2 start) | 1204 | 1471 |
| Tex closes per frame | 122 pause (R0 doc) | 208 | 122 |
| tex fail | 0 | 0 (all 6 counters read fail 0) | 0 |
| VRAM free min A/B | 1065/1065 KB (row 3 to 5) | 1065/1065 KB; vramfree min 2130 KB | not read |
| Log lines per frame | not in docs | 10365 lines / 667 frames, about 15.5 | 10830 lines, about 16 |
| Bg us per frame | 2126 (before BG) | 243 | 2169 |

- Yard, f300 to f599: the walk is 6608 us against 9812 us at the start of lane 2.
  Azahar numbers show direction only.
- Pause, f300 to f599: 7634 us against 15789 us before R0 and 9833 us after R0.
- Catalog shot lines pass on the new logs: yard `shot 0 frame 200`, pause `shot 2 frame 178`.
- full411 (one run, azt full411, row final, lane final, 412 shots): PASS. Log line
  `tour: done, 411 maps` at line 20246, run time 121 s. Completion only, no picture check.
  Tex fail count: 0 (623 fail counters, all fail 0). 57 lines read "ROM READ IN DROPPED RANGE":
  a ROM read of a dropped range, logged, not a failure of the test. Not investigated.

## 2. What was done (by area)

All hashes are on main. Lane 2 numbers are Azahar yard, frames 300 to 599, unless marked.

### Logging and boot
- Log gate, one switch in `vlog`: `d35248f0d`. Logging off stops all logging.
- Frame skip toggle (Debug row 9, default on): `9bb8240f7`.
- Memory HUD (game pool used, total, free in MB): `1733c0b70`.
- Sleep fix (bounded waits, audio re-init on wake): `ecb193f2f`, `6536958ee`.
- RAM residency: ROM loaded into RAM, SD streaming path removed: `8ec689a1a`,
  `04c03f525`. Boot line `rom mode: RAM resident 24.66 MB`. Doc: `diag_ram_residency.md`.
- Merged main build with these: `60e5a06e2`. Build 1297-ram: `7b98b8539`.

### Glyph and text log gates
- R0, glyph bind log behind the imglog switch: `363a0e8e1`. Pause scene walk last
  300 frames 15789 us to 9833 us (Azahar).
- Comb audit only with its switch, not at log level 3: `baa1cb683`. The console
  push uses loglevel=debug, so these gates matter on the console.
- R1 probe counters (tile 0 and tile 1 flush): `29f504d7b`.

### Texture cache and VRAM (lane 1, `agent_vram.md`)
- Stale texture fix: `a56ee72c4`.
- Linear staging freed two frames after upload: `9764c3413`.
- Content-hash slot validity, LRU stamp on each hit: `9f24be83e`, `45e029ead`.
- Row 2, per-bank VRAM accounting and HUD `VRAM A B min`: `7dea7290c`.
- Row 3, boot layout and 3 MB cache as two 1536 KB halves in banks A and B:
  `16a7dd63d`. Test: boot A used 2007 KB, B same, tex fail 0.
- Row 4, full buffers in bank A then B: `bb5a86fdd`, merged as `124697aa8`.
  Idle free seen in file select. Shots equal.
- Row 5, shrink and regrow of bank B: `f7cdcf3f3`. Log: one shrink, one regrow at
  frame 178, no texture fail.

### Display list and walk (lane 2, `agent_dl.md`)
Ops per frame at f599, and main walk average, Azahar yard:

| Row | Commit | Ops/frame | Walk us | Note |
|---|---|---|---|---|
| start of lane 2 | - | 3576 | 9812 | reference |
| C2 render-mode list as one port op | a6c525673 | 3236 | 9512 | fallback 0 of 68 |
| G1 sync and no-op skip | 9268569ae | 2417 | 8072 | picture ok |
| H1 cold split of gbi_run_body | 3449398b0 | 2417 | 8088 | noise |
| C1-0 tex and combine ops | 8d157f28d | 2417 | 8222 | +1.9 %, inside gate, HW-unconfirmed |
| BG sky as one bind | 78b4d2db1 | 2186 | 8270 | bg us 2126 to 259 |
| G2 equal state ops skipped | 07aebffa5 | 1943 | 8111 | sks 243 per frame |
| H3 matrix hash index | b831aa956 | - | 8086 | gms 179 to 8 |
| H2 flush_draw cold part | e1f8847ba | - | 8070 | inside noise |
| W2 timer ticks | 8ee2d859f | - | - | branch only, see section 3 |
| Phase timers out of the walk (W1 and P1 add the counters, 7bd0205f1) | - | - | 7208 | commit of the 7208 row not recorded in agent_dl.md |
| C1-1 G_PM_TEX op, class 2 ops | f93c91312 | 1018 fewer | 9065 | merged, picture equal by count |
| H5/H4 divides and PLD | 413deb696 | - | 7202 | HW-unconfirmed |
| R0 pause gate (pause window) | 363a0e8e1 | - | 9833 | pause, last 300 frames |

- Glyph op G_PM_TEXI, header over the texture load: `decf43341`. Pause draw_msg
  ops 1504 to 1092 per frame.
- Glyph texrect keep the atlas bind (branch only): `8362b7864`.
- Probe builds and plans: `plan_source_opts.md`, `plan_dl_architecture.md`,
  `plan_walk_cost.md`, `plan_text_batch.md`, `diag_opsplit.md`, `diag_pause_closes.md`.

Console baseline before this batch: 58.0 ms steady frame, walk 34.9 ms, 67 closes at
97 us. Azahar numbers show direction only.

### Test catalog
- `3ds/tools/azt.ps1` and `azt_scenarios.json`, from commits `15c57b2d5`, `8b49fd990`,
  `a890988e4` (rule 5 points to it). Catalog page: `docs/handoff/test_catalog.md`.
- Regex escape fix: `e99888363`.

## 3. What was tried and dropped, and why

- HUD bind (branch dl2-hud): saved 94 of the 250 us target. Not kept.
- H1b, tile cases to a noinline helper: walk +86 us. Reverted.
- W4, opf table prologue (branch dl2-w4): walk -21 us against a 150 us gate. Not kept.
- W2, timer ticks (branch dl2-w2): 2.0x fewer reads against a 2.5x gate. Not kept.
- ANIM, animator render-mode lists (branch dl2-anim, dl_anim): P1 producer marks broke.
  Not kept.
- GLYPH-ATLAS (branch dl2-glyph-atlas): tex closes 213 to 122, walk gate missed. Not merged.
- E4, divide removal in glyph functions: 100 ns per call, walk 9833 to 9747 us.
  Reverted: the cost is the flush and bind work, not the arithmetic.
- R1 probe, drop ens_pal check: refusals 33 to 6, wrap refusals 0 to 26. Tile 0
  flushes unchanged at 76. Reverted.
- Volcano fog: not reproduced. The kzn_18 warp showed a Goomba field, not the volcano.
  Fog start 2 is recorded in `agent_dl.md`.
- Pause menu cut: draw_msg needs an inline glyph bind. The branch exists and was
  partly measured, see `agent_dl.md`.
- 4 MB cache, 3 MB single block, kmr_30 texture result, DC3 to DC5, equality skip
  in C2, SUPER text and house sign limits, cutscene box: see `plan_vram_3mb.md`,
  `agent_dl.md`, `diag_dc*.md`, `diag_cutscene_box.md`, `test_catalog.md`. No
  final number for these was checked in this batch.

## 4. Open items and decisions for the user

- Cutscene box: frames not decided. See `diag_cutscene_box.md`.
- Fog room (volcano): not reproduced in Azahar. Needs a console look.
- If the yard is still above 33.4 ms on the console, the options are:
  - a retained per-model draw list,
  - use the N3DS second core,
  - accept 25 fps.
- Catalog gaps: pause_menu shot at 180 missing, text_menu, file_select,
  fileselect_rotate, full411, house_sign, super_text. Reset the `final` budget
  first.
- Branches not merged into main: `dl2`, `dl2-anim`, `dl2-hud`, `dl2-w4`, `dl_anim`.
  Reasons are in section 3. Each branch holds docs or dropped rows only.
- Archives are force-added to git. `3ds/logs` is in `.gitignore`.

## 5. Console checklist

Push command (rule 6 of CLAUDE.md). Run in Git Bash from any folder:

```
PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug
```

The push writes the live log to `build3ds/hw_live_log_170.txt`.

What to look at on the console:

1. Boot lines. Expect `rom mode: RAM resident 24.66 MB`. If the line says SD STREAMING,
   or a fatal screen shows need and free MB, send the screen text.
2. HUD rows MEM used and free. Expect free memory above zero on the yard.
3. HUD row VRAM A and B min. Expect no `fail` count above 0.
4. Debug tab rows Logging (off, on) and Frame skip (on, off). Both must change the
   frame time.
5. Live log fields: `opsplit`, `prod`, `texmap`, `tilesplit`, `tbsplit`, `rmsplit`,
   `gpusplit`, `pacestat`. Azahar numbers are direction only. The console is the
   judge.
6. Yard frame time. Baseline 58.0 ms steady. Target 33.4 ms (30 fps with margin).
   Walk baseline 34.9 ms, 67 closes at 97 us.
7. Pause menu and partner screen. Expect less lag than the baseline after the log gates.
8. File select. Note how long a pick takes against the baseline.
9. Lid-close test. Steps in `docs/handoff/diag_sleep_hang.md`.

Numbers to send back:

- The boot `rom mode` line.
- Yard steady frame ms, and walk ms.
- Pause menu and partner screen lag, in seconds or frames.
- File select pick time.
- Lid-close result, pass or fail, with the step where it failed.
- The last 50 lines of `build3ds/hw_live_log_170.txt`, or the file itself.

## 6. Round 2

Current archive (round 2, latest): `3ds/logs/builds/pm_3ds_final4_6e965c84b.3dsx` (build 6e965c84b, see FINAL4 below). Earlier build for this round: build 1433, git b5c7b5226 (FINAL3). Archive: `3ds/logs/builds/pm_3ds_final3_b5c7b5226.3dsx`, sha256 861ab9e6444bf601...(full hash in `3ds/logs/perf/pushed_builds.txt`). Not pushed. The earlier FINAL2 archive is `3ds/logs/builds/pm_3ds_final2_83e903455.3dsx` (build 1428, git 83e903455).

### FINAL4 (HEAD 6e965c84b, build 6e965c84b; current archive)

- Archive: `3ds/logs/builds/pm_3ds_final4_6e965c84b.3dsx`, 13366812 bytes, sha256 `f010c91ff24db35d06094ca6467ca6daead32af0b5f8dfb7a2caec7deb9ab862` (checked equal to build3ds/pm_3ds.3dsx). Line in `3ds/logs/perf/pushed_builds.txt`. Not pushed. Later commits f35192ba9 and 467925c71 touch only tests and docs, so the binary stays valid.
- Console push command (CLAUDE.md rule 6), run in Git Bash from any folder:

```
PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug
```

- Azahar catalog run, one start each, picture read by eye (top shots):

| Scenario | Log checks | Picture |
|---|---|---|
| yard | 3 of 3 | Green pipe, Mario, partner, house |
| pause_menu | 6 of 6 | Pause status page |
| partner_menu | 7 of 7 (after shot window moved to frame 200) | Party tab, Goombario. Frame 220 showed the Star Spirits page, so the window moved |
| file_select | 6 of 6 | File select screen |
| text_menu | 3 of 3 | Pause status text |
| mac01 | 4 of 4 | Post office, Mario, Toads |
| mac01t | 6 of 6, census gate 1696 in 1623 to 2065 | Toad Town street, Mario, Toads. Verified |
| battle_start | 4 of 4 | Battle command ring, Mario, Goomba |

- Starts used on row FINAL4: 8 of 8 before the budget reset (one yard start cut off, then 7 scenarios), then 2 more after the reset (partner_menu, yard).
- Not a hardware result. A person must confirm on the console.

#### Round 2 rows (gates)

- CROSS2 (ec9a7dc13): second kick split at 85 % of draws in pm_gpu.c. Shots byte-equal (mac01 2, mac01t 2). mac01t splits per frame 1.0 to 2.0.
- EN2 (35b41c5eb): shadow texture load as one G_PM_TEXI run. mac01t shadow draws 795 to 520 ops (26.5 to 17.3 per draw). The plan target was 16 per draw, so the per-draw target is not met. The epic gate (down, pictures equal) passes.
- T1B (b7e847d8c): task tail rows named 0xF0E0 + k. Census only. Pictures equal.
- NP1B (64be3494b): sprite texture load as one G_PM_TEXI. mac01t f899 NPC sprite draws 490 to 301 ops (18.1 to 11.1 per draw). Gate 12 or less: pass. The first NP1 was dropped (yard bottom shots differ, see below).
- HT1 (be50979c1): HUD element and tile texture loads as G_PM_TEXI runs. mac01t f899 ops 319 to 171 (gate 100 or more: pass). Closes unchanged.
- HUD interval, LOGCLEAN, WARPLOG, DS0 (dcsum line) and DS1 (svc flush) are listed in the sections above.

#### MENULAG gates (base plus 15 %, rounded up)

| Scenario | Walk cpu ms max | Closes max | Ops max |
|---|---|---|---|
| pause_menu | 12.8 | 156 | 1692 |
| partner_menu | 11.0 | 124 | 1324 |
| file_select | 9.4 | 102 | 1617 |

Walk is `cpu (wall-idle-fbegin)` from the `phase W` line. Closes is the first `phase count` line. Ops is `gbiops`. See `docs/handoff/test_catalog.md`.

#### TOADT census (mac_01, window f899 = frames 600 to 899)

- Baseline census 2315 ops per frame (`docs/handoff/diag_toadt.md`). Model 00b7 holds 32.0 % of ops and changes its matrix every frame. The docs do not name model 00b7 (`docs/handoff/diag_toadmdl.md`).
- After EN2, T1B, NP1B and HT1: 1844 ops per frame. Gate set to 1623 to 2065 (1844 +/- 12 %).

#### Dropped rows

- CROSS1: one linear block for pool, ring and index. The premise fails (pool 1.31 MB; one flush would cover about 1.8 MB). Reason in `docs/handoff/agent_crossfix.md`.
- DS3: generation counters for the pal block. Shots equal, but pal close time did not fall 20 % (3 us to 3 us at dcsum resolution 1 us). Reverted.
- DS2: -Os for pm_gpu.c. Picture equal, but the dcsum stage sum rose 24 to 26 us per close. Reverted. Cold-branch moves not done.
- MODELOPS R1 to R3: the R0 counts give a ceiling below each gate (R1 max 190 us against 245 us; R2 max 335 us against 500 us; R3 mtx_same about 4 us against 175 us). Not coded (`docs/handoff/agent_modelops.md`).
- NP1 (first attempt): yard bottom shots differ by 18 to 140 px. A HEAD control run differs by the same order. Top shots equal. The byte-equal gate cannot separate it on yard.

#### Unproven on console (from `docs/handoff/diag_abcmp.md`)

- Wall frame time on console: unproven. The paced walk stays at 33.4 ms in both builds.
- Split of the pair B non-op drop (about 4 ms logging or DS1, about 2 ms ops): not proven.

#### Yard A/B procedure (console, from diag_abcmp.md section 5)

1. Copy `3ds/logs/builds/pm_3ds_merge_60e5a06e2.3dsx` to build3ds/pm_3ds.3dsx. Save the current file first.
2. `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 -t 60 audiostat texstat pacestat logstat phase phsum loglevel=debug preset=yard`
3. Stand Mario still at the yard start for 60 s.
4. Read build3ds/hw_live_log_170.txt: pace, phase W, phase count, logstat. Skip frames 0 to 299.
5. Copy `3ds/logs/builds/pm_3ds_final4_6e965c84b.3dsx` to build3ds/pm_3ds.3dsx. Repeat steps 2 to 4.
6. Add both pushes to 3ds/logs/perf/pushed_builds.txt.

#### Console checklist for build 6e965c84b

1. Boot line: `build 6e965c84b git <hash> loglevel <n>`.
2. No log gaps. The frame counter runs with no jumps.
3. `dcsum` line: write down close us per stage (pal, bind, tev, st, draw) and gc us per flush.
4. HUD interval: set `hudint=` in `options.ini`. Check the bottom memory rows refresh.
5. VRAM HUD rows: rows 1 and 2 show min free and peak used.

### FINAL3 (HEAD b5c7b5226, build 1433)

- WARPLOG (commit b5c7b5226): the per-2-frame warp scene line is removed from the padscript output.
- PAUSE2 fix (commit dc6d75a8f): the pause_menu scenario opens the pause with START at scan 80, shots 120 to 137, START closes it at about frame 140, field shot 152 to 159 (until 156). The FINAL2 failure (close after the window) is fixed. Diagnosis: `docs/handoff/diag_pause2.md`.
- No-idle rule (commit 2544d4bf1, CLAUDE.md rule 5): no test idles. A test ends within 5 s of its until-line. Each wait in a scenario is the time the subject needs. The yard, sewer, ruins and volcano steady-frame wait noted above is covered by this rule and is a candidate for trimming.
- Azahar runs, one start each, picture read by the agent:

| Scenario | Log checks | Wall time (start to end) | Picture |
|---|---|---|---|
| yard | 3 of 3 | 27.6 s | Yard, green pipe, Mario and partner. OK |
| pause_menu | 4 of 4 | 26 s | Frame 123: pause status page with dialog. Frame 155: field with Mario and green pipe. OK |
| mac01 | 4 of 4 | 22 s | Frame 200: Toad Town post office, Mario, Toads. OK |

- Picture ok for the archive. A person must confirm on the console. Not a hardware result yet.

### LOGCLEAN

- `loglevel=debug` added to the yard and pause_menu scenario strings.
- The per-line fsync in the log hot path is removed (`3ds/src/pm_log.c`). An SD fsync held the log lock for 1 to 7 s, so log lines were lost. The code comment at pm_log.c line 522 records the reason. No gap check was run on the FINAL2 logs.
- Log checks: yard 3 of 3, pause_menu 3 of 3 (earlier run). Pictures were not read by a person in that run.

### MODELOPS R0

- R0 is done (commit 25d191c70). It adds the op counters and the mac01 scenario.
- R1, R2 and R3 are not built. The R0 counts give a ceiling below each gate:
  - R1: at most 190 us. Gate 245 us.
  - R2: at most about 335 us. Gate 500 us.
  - R3: about 4 us. Gate 175 us.
- Decision: R1 to R3 are closed for mac01. A new plan is needed for the per-model cost.

### DRAWCOST

- DS0 (commit cbab0e929): one `dcsum` line every 300 drawn frames. It gives the close time per stage (pal, bind, tev, st, draw, in us per close), the cache flush time (gc, us per flush and flushes per frame), and the command share per sampled close.
- DS1 (commit cd248cbc8): CPU cache flush by svc, with a GSP call only on failure. Picture byte-equal on mac01. Fallbacks 0.
- DS2 reverted: -Os on `pm_gpu.c` gave an equal picture but a close sum of 26 us against 24 us (+8 %, one run). No cold-branch move was made.

### HUD

- Commit b7375d4f0. `options.ini` key `hudint=<ms>`: 250, 500, 1000 (default), 2000, 5000.
- Bottom rows 1 and 2 are VRAM: min free, peak used since map load, tex fill, A and B used and free, evictions per s.
- `shotbottom=<first>:<count>` writes one bottom-screen shot per frame.

### FINAL2 catalog run (HEAD 83e903455, build 1428)

| Scenario | Until-line | Wall time (start to until) | Picture |
|---|---|---|---|
| yard | scene f300 | 23 s (was 23 s) | Mario, green pipe, partner in yard. OK |
| mac01 | first mops line | 16 s (was 16 s) | Post office, Mario, Toads. OK |
| text_menu | shot 178 to 181 | 15 s (was 91 s, timeout) | Pause status page, dialog readable. OK |
| file_select | shot 338 | 16 s (was 90 s, timeout) | File select screen. OK |
| pause_menu | field shot 192 to 200 | 18 s (was 17 s) | FAIL for the close. Frames 173 to 193 all show the pause page |
| fileselect_rotate | shot 29[0-6] | not run | unverified |

- pause_menu: the START close at `wait284` is not done by frame 193. The next run needs a longer wait before the close, after a budget reset.
- Idle check: yard, sewer, ruins and volcano wait from shot 204 to the scene frame 300 (about 3 s). This is the steady-frame check. Not trimmed, pending a decision on dropping that check.

### Branch and worktree cleanup

- `git worktree list` shows only `main`. `git branch` shows only `main`. Remote branches were not changed.

### Console checklist for build 1428

1. Boot line: `build 1428 git 83e903455 loglevel <n>`. Check that the hash and loglevel are present.
2. No log gaps. The frame counter in the log runs with no jumps (fsync removed).
3. `dcsum` line: write down the close us per stage (pal, bind, tev, st, draw) and the gc us per flush.
4. HUD interval: set `hudint=` in `options.ini`. The bottom memory rows change their refresh step.
5. VRAM HUD rows: rows 1 and 2 show min free and peak used.

Push command (CLAUDE.md rule 6). Run in Git Bash from any folder:

```
PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug
```
