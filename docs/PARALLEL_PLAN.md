# Parallel plan (written 2026-10-18, read-only study, nothing was run)

Sources read: ORCHESTRATOR.md, HANDOFF.md, SLICES.md, INDEX.md, run_azahar.ps1, build.sh, go.ps1, qt-config.ini, the Azahar folder. Facts marked UNVERIFIED were not tested. X02 owns Azahar and the build now.

## Part A. Can several Azahar instances run?

Answer: YES in principle. Each instance needs its own user directory. The repo does not do this today. UNVERIFIED: no run was made.

### A1. Separate user directory per instance
| Item | Finding |
|---|---|
| Portable mode | Azahar (a Citra fork) uses a folder named `user` next to azahar.exe as its user directory, if the folder exists. This is Citra behaviour. UNVERIFIED for the installed version (first step of the setup tests it). The repo has no portable folder now. `C:\Program Files\Azahar` has no `user` folder. |
| Command line option | Not needed and not allowed. The rule says azahar.exe starts with only the .3dsx path. Portable mode needs no option. |
| Per-instance copy | Copy `C:\Program Files\Azahar` (178 MB) to `C:\azahar-lanes\<lane>\`. Add `user\config\qt-config.ini` (copy of the real one, edited), `user\nand` (177 MB copy, first boot needs it), `user\sysdata` (4 KB), `user\sdmc\3ds\PaperMario\papermario.z64` (40 MB, use a hard link: `mklink /H`), `user\shaders` (11 MB, optional, avoids a cold shader cache). Do NOT copy the 4.5 GB real sdmc. About 430 MB per lane. |
| Config keys to change per lane | `gdbstub_port` (24689 is fixed in the script), `nand_directory` and `sdmc_directory` (set to the lane paths, or leave them to portable mode), `use_custom_storage`. |

### A2. Shared paths in a run
| Path or resource | Where it is used | Shared? | Fix per lane |
|---|---|---|---|
| `%APPDATA%\Azahar\config\qt-config.ini` | run_azahar.ps1 `$cfg` (Set-Turbo, Set-GdbStub) | Shared. Overlap races (T02 cause 3). | Lane config in `user\config`. |
| `%APPDATA%\Azahar\sdmc\3ds\PaperMario` | `$sd`: 3dsx copy, debug3ds.txt, log.txt, shots, save.flash, crash.txt, ROM | Shared. One game log, one debug3ds.txt. | `$sd` under the lane `user\sdmc`. The game reads `sdmc:/` so it follows the lane. |
| `%APPDATA%\Azahar\log\azahar_log.txt` | `$emuLog`: FaultStop, exception, last_unmapped | Shared. Run start deletes it. | Lane `user\log`. |
| `build3ds/` and `pm_3ds.3dsx` | build.sh output, run_azahar.ps1 copies the 3dsx, last_log.txt, last_unmapped.txt, last_exception.txt, last_shot.png, last_gdb.txt, gdb_cmds.txt, qt-config.turbo-backup | Shared inside one tree. | A git worktree per lane gives each lane its own `build3ds/`. |
| Docker volume `pm3ds-work` (`/work/src`, `/work/build`) | build.sh rsync with `--delete`, then cmake. gdb and addr2line in run_azahar.ps1 read `/work/build/pm_3ds.elf` | Shared. Two builds destroy each other. | Volume per lane: `pm3ds-work-<lane>`. Change build.sh and the two `docker run` calls in run_azahar.ps1 to use `$PM_LANE`. |
| `build.sh` last step | copies the 3dsx to the real `%APPDATA%` sdmc | Shared. | Skip it when `PM_LANE` is set. run_azahar.ps1 copies the 3dsx anyway. |
| `debug3ds.txt` | written to `$sd`, removed in finally | Shared. | Follows `$sd`. |
| `3ds/logs/` | git-ignored, agent logs and traces | Per worktree if the lane has its own tree. | Use the lane tree. |
| `3ds/golden/*.png`, `3ds/tools/golden.py` | tracked references, calls run_azahar.ps1 | Tracked files. Only one lane may run `--update`. | golden.py inherits `PM_LANE` from the environment. |
| Capture by window | `-Capture`: `$p.MainWindowHandle` and PrintWindow. The dialog check uses the window list of the PID. | Per PID, not by title. | None. |
| Game shots (`shots` switch) | PPM files in `$sd\shots`, copied to `build3ds\shots` | Follows `$sd`. | None. |
| GDB port 24689 | Set-GdbStub | Shared. | Port per lane in the lane config. Pass it to gdb (`target remote winhost:<port>`). Rarely used. |
| Live log TCP 17492 | `pm_log.c` binds this port inside the game. Azahar guest sockets use the host network. | A second instance gets `bind 17492 failed`. The game logs the line and goes on. UNVERIFIED. | None needed. Ignore the line. |
| `PROGRESS.log`, `HANDOFF.md`, `SLICES.md`, `docs/*` | tracked | Merge conflicts only. | Rule: a lane appends only. The orchestrator merges. |
| Kill logic | Kill-Azahar uses the PID. | Safe already. | None. |

### A3. Does shot capture depend on one window or on focus?
- No. `-Capture` uses the PID of the launched instance, `PrintWindow` (works without focus) and `GetWindowRect`. The game shots come from the game as files. The script never uses a window title.
- Risk: PrintWindow of a minimised window gives a black frame. Keep the windows open and not minimised.

### A4. What N lanes need
1. A git worktree per lane: `git worktree add C:\pm-lanes\<lane> -b lane/<lane> main`. The existing `3ds/logs/wt_battle` worktree is old (detached). Remove it first or ignore it.
2. The extracted assets are git-ignored (`ver/us/build`, `src/vtx`, `assets`, `include/message_ids.h`). A new worktree does not have them. Copy or junction them from the main tree (`mklink /J`). This is the largest hidden cost. Check it first.
3. A Docker volume per lane (`pm3ds-work-<lane>`). The first build in a lane is a full build. The ninja build uses all 32 threads, so parallel builds slow each other.
4. An Azahar folder copy per lane with its own config, sdmc, nand, log (A1).
5. A lane variable `PM_LANE` read by build.sh, run_azahar.ps1, golden.py. Empty means the old behaviour.
6. Merge back: each lane commits on its branch. The orchestrator merges into main one lane at a time (`git merge --no-ff lane/<lane>`), builds once and runs the smoke check on `yard` and `toad`. The shared files (HANDOFF.md, PROGRESS.log, SLICES.md) conflict. Lane agents append only. The orchestrator fixes conflicts.
7. Generated tracked files (maps.cmake, map_syms.lds, stub_include) conflict if two lanes regenerate them. Only the S30b lane may run generators.

### A5. Fallback: one lock for the Azahar run
| | Per-lane Azahar (A4) | Lock only |
|---|---|---|
| Parallel edit and build | Yes, if build dirs and volumes are separate | Yes, only with a worktree and a volume per lane. Without them builds still clash. |
| Parallel Azahar run | Yes | No. Agents queue. A run is 16 to 60 s. |
| Change size | Large: 3 scripts, a setup script, an unverified portable mode | Small: a named mutex in run_azahar.ps1 (about 10 lines), part of T02 |
| Risk | Portable mode may not work. The lane config can drift. | Low |

Recommendation: do the lock FIRST (T02, small, safe). Add worktree, volume and build dir per lane next, because builds are the long step. Add the per-lane Azahar copy only if the queue for Azahar becomes the bottleneck. Most slices run Azahar for about 1 to 3 minutes in total, so the queue is short.

### A6. Cost of one instance
- Not measured in the repo or the logs. The logs only give guest numbers (audio 3 ms per tick, guest memory 98 MB offered). An Azahar run uses CPU for the emulator thread and the render thread. At `-Turbo` the speed limit is 1000 percent, so it takes as much CPU as it can get.
- Estimate, UNVERIFIED: 2 to 4 cores and 1 to 2 GB RAM per instance (process working set, window, shader cache). Measure with `Get-Process azahar | select WS,CPU` during one run.
- The PC: Ryzen 9 3950X, 16 cores, 32 threads, 64 GB RAM. Three or four instances fit. Docker builds also use all threads. Turbo runs from several lanes at the same time may change timing but not frame-driven results. No performance numbers from a parallel run are valid.

### A7. Changes in run_azahar.ps1 (relates to row T02)
| # | Change | T02 link |
|---|---|---|
| 1 | Named mutex `Global\pm_azahar_cfg` around any read or write of a config. Release in `finally`. | T02 named mutex |
| 2 | Resolve paths from `$env:PM_LANE`. Empty: the paths of today. Set: `$exe`, `$cfg`, `$sd`, `$emuLog` under `C:\azahar-lanes\<lane>`. | T02 per-run config |
| 3 | Use the lane config only. The real `qt-config.ini` is never opened when a lane is set. Later also for the default run (copy the real one to a temp user dir). | T02 core goal |
| 4 | Wrap the whole body in try/finally. Move Set-Turbo and Set-GdbStub inside the `try`. | T02 cause 1 |
| 5 | Remove the unconditional `Set-Turbo $false` at line 109 for a lane run (it deletes a running lane's backup). | T02 cause 3 |
| 6 | Fail loudly when a config key is missing (throw, do not skip). | T02 cause 5 |
| 7 | Docker calls (gdb, addr2line): volume `pm3ds-work-$PM_LANE`. Gdb port from the lane config. | new |
| 8 | Optional: when a lane has no config, copy from the real one and set `use_gdbstub`, `frame_limit`, `use_vsync` once. | new |
| 9 | While X02 runs, do not edit the file in place. Edit it after X02 ends, or write `3ds/run_azahar_lane.ps1` and merge later. | safety |

## Part B. Lookahead over open rows

Shelved: V09 intro tints and nok_01 (first V09 row). Optional (unconfirmed visual faults): V09 kmr_22, V10, V11.
Model tier by the MODEL RULE: opus only for plans, sonnet for code, shared code, graphics, memory, audio, haiku for mechanical work.

| Row | Files or subsystem (likely) | Azahar runs | User or hardware | Tier |
|---|---|---|---|---|
| S28 | no code. Hardware run, boot line, audio cost, fps | Not Azahar. O3DS and N3DS | YES (O3DS on, dspfirm.cdc) | sonnet (judgment on numbers) |
| S30b | `3ds/maps_skip.txt`, `maps.cmake`, `map_syms.lds`, `CMakeLists.txt`, `map_restore.*`, `src/world/area_*`, memory layout | yes, tours | YES, answer to Q7 | sonnet |
| V07 | `src/entity.c`, `pm_gbi.c`, `testitem` in `pm_warp.c` | yes | YES, exact scene or hardware | sonnet |
| V06 | `pm_gbi.c`, `pm_gpu.c` (fog, blender) | yes | YES, user must confirm | sonnet |
| X02 | `pm_gbi.c`, `pm_gpu.c`, `pm_tex.c` (render to image, 32 KB VRAM, 3 KB code) | yes (running now) | no | sonnet |
| S31 | `game_stubs.c`, `pm_later.c`, crash handler in `pm_main.c`, `crashtest*` switches | yes (`crashtest_call`) | partly: real crash screen needs hardware | sonnet (no-stub judgment) |
| F01 | kmr_23 `src/curtains.c`, tst_12 cut map, nok_01 `src/audio/core/engine.c` (the nok_01 part overlaps shelved V09), pause, save, zones | yes (tour) | no | sonnet |
| F03 | `src/model.c:4497`, depth read back in `pm_gpu.c` and `pm_gbi.c` | yes (`warp=hos_05`) | no | sonnet |
| T02 | `3ds/run_azahar.ps1`, `build.sh`, `golden.py` | one check run at the end | no | sonnet (script plus config safety) |
| F08 | `pm_gbi.c`, `pm_comb.c`, `pm_gpu.c`, shader `pm.v.pica` | yes, fog map | no | sonnet |
| PO2 | `pm_gbi.c` (vertex transform at G_VTX), `pm_phase.c` | yes (phase timer) | YES, answer in QUESTIONS.md | sonnet |
| PO3 | plan in `docs/`. Later `pm_gbi.c`, `pm_gpu.c`, `pm.v.pica` | plan: no. Later: yes | no | opus for the plan, sonnet later |
| AU1 | plan first. Later `pm_audio.c`, `pm_rsp_audio.c`, `pm_os.c`, `src/audio/core/engine.c` | later yes | YES for the O3DS cost number | opus plan, sonnet later |
| MA1 | many files (libm calls, divides). Replay hash in `pm_warp.c` | yes (replay check) | YES, approval for game-logic math | haiku for the listing, sonnet for changes |
| F06 | plan only, after S28 | no | needs S28 | opus |
| F07 | plan only, after S28 | no | needs S28 | opus |
| S18 | check run, move row to `docs/SLICES_DONE.md` | yes, one run | no | haiku |
| S19c | check one attack battle, move row | yes, one run | no | haiku |
| V09 kmr_22 (optional) | card texture in `src/world/area_kmr/kmr_22`, `pm_tex.c` | yes | no, but unconfirmed fault | sonnet |
| V10 (optional) | `draw_box` pattern path `box_pattern_rect`, `pm_gbi.c` | yes | no, unconfirmed | sonnet |
| V11 (optional) | kmr_05 collision or map data | yes | no, unconfirmed | sonnet |

### Batches without file conflicts
Conflict groups: GFX = `pm_gbi.c`, `pm_gpu.c`, `pm_tex.c`, `pm_comb.c`. AUDIO = `pm_audio.c`, `pm_rsp_audio.c`, `pm_os.c`, `engine.c`. MAPS = `maps.cmake`, `CMakeLists.txt`, generated files. TOOLS = `run_azahar.ps1`, `build.sh`, `golden.py`.

| Batch | Rows | Why they do not conflict | Needs |
|---|---|---|---|
| 0 (beside X02, now) | T02 (TOOLS, edit in a copy or after X02), PO3 plan (docs only), AU1 plan (docs only), MA1 listing (docs only) | No GFX file. No build. No Azahar run. Plans write one new file each in `docs/`. | no build, no Azahar |
| 1 (after X02 ends and lanes exist) | F01 (curtains, engine.c, tst_12), S31 (stubs, crash handler), S18 and S19c (docs plus one run each) | Different files. F01 touches AUDIO `engine.c`, so no AU1 coding in this batch. | one lane each |
| 2 (GFX chain, one lane at a time) | F03, then F08, then PO2 (needs the user answer), then optional V10, V09 kmr_22 | All touch `pm_gbi.c` or `pm_gpu.c`. They would conflict. | one lane |
| 3 (AUDIO) | AU1 code after its plan, after F01 nok_01 | Shares `engine.c` with F01. | S28 number helps |
| 4 (sequential, alone) | S30b | Changes memory layout and generated files. All other measurements and golden frames move. Needs Q7. | user |
| 5 (user or hardware) | S28, V06, V07 | Cannot run without the user. S28 first because it settles Q2 and Q7. | user |
| 6 (late) | MA1 changes, PO3 code, F06, F07 | Touch many files. MA1 needs replay hashes and approval. F06 and F07 plan after S28. | S28, user |

Rows that must stay sequential: S28 first in the hardware path. S30b after Q7 and alone. The GFX chain (X02, F03, F08, PO2, PO3 code) because of shared files. MA1 changes and PO3 code after the GFX chain. F06 and F07 after S28.

### What can start beside X02 now
- T02 (write the changes in `3ds/run_azahar_lane.ps1` or wait: X02 runs `run_azahar.ps1`). No Azahar run, no build.
- PO3 plan, AU1 plan (opus, docs only), MA1 listing (haiku, read-only listing into one doc).
- Read-only analysis for F01, F03, S31 (cause notes only, no build, no code edit).
- NOT now: anything that builds (the shared `pm3ds-work` volume and `build3ds/` belong to X02) or runs Azahar, and any edit in the GFX files.

## Recommended setup
1. Phase 1 (now): the lock. T02 adds the named mutex, try/finally, and loud key checks. No lanes yet.
2. Phase 2: worktree, Docker volume and build directory per lane (2 or 3 lanes). Lanes share one Azahar through the mutex. Runs queue.
3. Phase 3 (only if the queue hurts): per-lane Azahar folder with portable `user` directory (A1).
4. Merge: orchestrator merges one lane at a time, smoke check `yard` and `toad` after each merge.

## First steps for the build agent (haiku or sonnet, after X02 ends)
1. Read ORCHESTRATOR.md, this file. Check `git status`. Do not touch X02 files.
2. Test portable mode on a throw-away copy: `robocopy "C:\Program Files\Azahar" C:\azahar-lanes\t1 /E`, make `C:\azahar-lanes\t1\user`, copy `%APPDATA%\Azahar\config`, `nand`, `sysdata` into it, hard link the ROM into `user\sdmc\3ds\PaperMario`. Start `C:\azahar-lanes\t1\azahar.exe` with ONLY the 3dsx path (use a copy of the script with the lane paths). Check that `user\log\azahar_log.txt` and `user\sdmc\...\log.txt` appear and that `%APPDATA%\Azahar\log\azahar_log.txt` does not change. Write the result (works or not) in PROGRESS.log. If not, stop at Phase 2.
3. Add `PM_LANE` to `build.sh` (volume `pm3ds-work-$PM_LANE`, no copy to the real sdmc when set). Add the same volume name to the two `docker run` calls and the lane paths in `run_azahar.ps1` (A7 items 1 to 7).
4. Create lane worktrees: `git worktree add C:\pm-lanes\L1 -b lane/L1 main` and `L2`. Junction the git-ignored assets (`ver\us\build`, `src\vtx`, `assets`, `include\message_ids.h`) from the main tree. Check `include/message_ids.h` is regenerated or copied.
5. In each lane: `set PM_LANE=L1`, run `sh 3ds/build.sh`, then `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=yard"`. Both lanes in parallel. Both must exit 0 and keep their own `build3ds/last_log.txt`.
6. Measure CPU and RAM during the parallel run (`Get-Process azahar | select Id,WS,CPU`). Write the numbers in this file (Part A6).
7. Write the lane rules in ORCHESTRATOR.md (one paragraph): lanes append only to shared docs, the orchestrator merges, only the S30b lane runs generators.
8. Commit locally in plain English. No push. No attribution.
