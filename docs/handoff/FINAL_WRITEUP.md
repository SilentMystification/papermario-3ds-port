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
| pause_menu | pause mode pass. Shot at frame 180 not logged | Status page, dialog text | FAIL (log) |
| partner_menu | 4 of 4 pass | Party tab, Goombario | pass |
| battle_start | 4 of 4 pass | Battle HUD, Jump menu | pass |
| sewer | 3 of 3 pass | Mario in tunnel | pass |
| ruins | 3 of 3 pass | Mario, text box | pass |
| volcano | 3 of 3 pass | Mario in lava room | pass |
| text_menu | not run (same command as pause_menu) | from the pause_menu shots | not run |
| file_select | not run (budget) | not read | not run |
| fileselect_rotate | not run (budget) | not read | not run |
| full411 | not run (budget) | not read | not run |

- Yard log numbers for this batch were not captured. The phase lines were not on
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
