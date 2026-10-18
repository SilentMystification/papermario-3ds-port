# S28 prep: first hardware run (O3DS 192.168.1.168, N3DS 192.168.1.223)

Written 2026-10-18. Read-only preparation. Nothing was built or run. Sources: 3ds/SLICES.md row S28, HANDOFF.md, docs/MEMORY_BUDGET.md, docs/plans/AU1.md, PO3.md, buffer_bridge.md, `.claude/commands/{push,pull}-{o,n}.md`, 3ds/go.ps1, 3ds/push_3ds.sh, 3ds/tools/ftppull.py, 3ds/src/pm_main.c.
Tags: CODE = read in source. UNV = not verified on hardware.

## 1. Checklist for the user (before the run)

| # | Item | Detail |
|---|---|---|
| 1 | Console on, charged or on mains | Test the O3DS first. The N3DS comes after. |
| 2 | Same Wi-Fi as the PC | The PC pings 192.168.1.168 and 192.168.1.223. |
| 3 | `sdmc:/3ds/dspfirm.cdc` | CODE: `pm_audio.c` checks this exact path. Without it the game runs silent and the audio numbers are not valid. |
| 4 | ROM `sdmc:/3ds/PaperMario/papermario.z64` | CODE: big-endian .z64, US. Else the bottom screen shows "ROM missing" and the game stops. The game creates the folder. |
| 5 | Free SD space | About 40 MB for the ROM, about 11 MB for the 3dsx, a log file. Keep 200 MB free. |
| 6 | 3dsx size | The file `build3ds/pm_3ds.3dsx` is 10,921,876 B (10.9 MB) now. You said about 13 MB. The working tree has unstaged changes, so the push will build again. The size can change. Not a problem, the netloader sends it by Wi-Fi. |
| 7 | Luma settings | Hold SELECT at boot. Enable "Show NAND or user string in System Settings" is not needed. Enable game patching is not needed. The exception handler must be on (default in Luma 8+). Luma then writes `sdmc:/luma/dumps/arm11/*.dmp` on a crash. Create the folder `sdmc:/luma/dumps/arm11` if it does not exist (UNV). |
| 8 | Homebrew Launcher netloader | For `push`: open Homebrew Launcher, press Y. The screen shows "Waiting for 3dslink". The PC finds the console by UDP broadcast on port 17491. |
| 9 | FTP server for `pull` | `pull` needs an FTP server app on the console, anonymous login, port 5000. Start it after the game closes. Netloader and FTP server are two separate apps. |
| 10 | Docker Desktop running on the PC | `go.ps1` builds and sends with the devkitpro docker image. |
| 11 | Remove old `debug3ds.txt` from `sdmc:/3ds/PaperMario/` and the 3dsx folder | A stale `fast` line changes the boot. Switches go as game arguments (section 3). CODE: the game also reads `debug3ds.txt` from the working directory at start. |
| 12 | One console at a time | `pull` does not do two consoles. Netload by two consoles at once is possible but do not do it in S28. |

Answer for QUESTIONS.md "S28: is the O3DS ready": the user states it when items 1 to 5 hold.

## 2. Commands in order

All run from the repo root. Each runs in the background. The skills run the same commands.

### 2.1 O3DS run

| Step | Action | Command or skill |
|---|---|---|
| O1 | Console: Homebrew Launcher, press Y | by hand |
| O2 | Run A: boot line only, no extra switch | skill `push-o` = `powershell -NoProfile -File 3ds/go.ps1 push o -Seconds 75` |
| O3 | Close the game (HOME, close). Start the FTP server. | by hand |
| O4 | Pull | skill `pull-o` = `powershell -NoProfile -File 3ds/go.ps1 pull o`. Output: `build3ds/hw_o3ds_log.txt`, `build3ds/hw_o3ds_dumps/` |
| O5 | Repeat O1 to O4 for runs B to F (section 3) with `-GameDebug "<switches>"` and the listed `-Seconds` | `push-o` passes `$ARGUMENTS`, so `/push-o -GameDebug "..." -Seconds 90` works |
| O6 | Read the live echo: `build3ds/hw_live_log_168.txt` (one file per console, overwritten per run) | Copy it to `build3ds/s28_o_<run>.txt` after each run, before the next push |

Rules from go.ps1 and push_3ds.sh (CODE): the build runs first, once. A netload that already started is never killed. A console that is not in the netloader gets 3 attempts of 15 s. If it fails, press Y again and rerun.

### 2.2 N3DS run

Same steps with `push-n` (`go.ps1 push n`) and `pull-n` (`go.ps1 pull n`). Live file: `build3ds/hw_live_log_223.txt`. Pull files: `build3ds/hw_n3ds_log.txt`, `build3ds/hw_n3ds_dumps/`. Run only runs A, C, D, E (section 3) on the N3DS. The N3DS enables the speed-up (`osSetSpeedupEnable(true)`, CODE) and has 124 MB application memory (UNV), so its memory lines are not the limit for the O3DS.

## 3. Debug switches per run

Pass them with `-GameDebug "..."`. They go to the game as arguments and merge with `debug3ds.txt`. The merged string is at most 255 characters (CODE: `sw[256]`). Never use `fast` alone for a title run: it skips the logos.

| Run | Switches | Length | Scene | Measures |
|---|---|---|---|---|
| A boot | none | `-Seconds 75`, stop after the title shows | logos, title | boot lines: `MODE`, `mem commit limit`, `mem split H03`, `dspfirm.cdc`, `rom resident`, `threads`, `APT_SetAppCpuTimeLimit` |
| B title | `audiostat pacestat logstat` | 60 s, no `fast` | title with music (about 2 s per `audio cost` line, 1 s per `pace` line, 300 frames per `logstat` line) | audio per tick (title), steps per second |
| C toad | `fast preset=toad audiostat pacestat logstat` | 90 s | Toad Town, music and sound, player controllable. No pad input needed. | audio per tick, steps per second, linear use |
| D yard | `fast preset=yard audiostat pacestat` | 60 s | Mario's yard (kmr_20), the slowest GBI scene (15.3 ms in Azahar, PO3) | steps per second worst case |
| E phase | `fast preset=toad phase phasecsv` | 60 s | Toad Town | phase split. Then `python 3ds/tools/phase_report.py build3ds/hw_o3ds_log.txt` |
| F battle | `fast preset=yard battle audiostat pacestat` | 60 s | a battle | audio per tick in battle. UNV: `battle` with `preset` was not tested in this prep. Fall back to `battle=<f>:<s>` from `docs/INDEX.md` line 49 if the warp fails. |
| G crash test (optional, last) | `fast preset=toad crashtest` | 20 s | forced crash | proves the Luma dump path and `[CRASH]` lines. Run it only after A to F are saved. |

Phase CSV in run E is large. `phasecsv` writes one line per drawn frame. 60 s is about 1,800 lines. Do not use `logflood`. Do not use `lognull` (it hides the lines).
Do not run `phase` together with `audiostat` and `pacestat` in one run: the phase timer adds cost (CODE: `pm_ph` ticks) and it biases steps per second.

## 4. Log lines to collect and what they decide

Source: `build3ds/hw_live_log_<ip>.txt` while it runs, `build3ds/hw_<o3ds|n3ds>_log.txt` after the pull (`sdmc:/3ds/PaperMario/log.txt`). Read with `grep` on the strings below. Never read a whole log.

| Line (grep string) | Run | Number | Decides |
|---|---|---|---|
| `MODE =` | A | O3DS or N3DS, console region MB | Which console the log came from. Check 64 MB cap text. |
| `mem commit limit` | A | commit limit KB, used-by-image KB, offered KB | Real memory limit against the 64 MB (65,536 KB) assumption. Offered KB = limit - image (about 17,000 KB image). Q7 and MEMORY_BUDGET section 8. |
| `mem split H03` | A | linear pool 14,336 KB, fixed need 7,528 KB, tex budget 4,096 KB, margin 2,712 KB, heap KB | Real heap total. MEMORY_BUDGET estimate: heap 34,200 KB. Spare = heap - 27,543 KB (ROM plus malloc at map load). Estimate 6,657 KB with 271 maps. |
| `rom resident` | A | 27,106 KB | Boot cost, resident ROM fits. |
| `mem boot stack high-water` | A | game, audio stack use | Stack size margin. |
| `mem ... linear used ... peak ... texture failures` | C, D, F | linear used, peak, failures count | VRAM and linear use (texture cache). Failures above 0 = H03 skips draws. Fragmentation: peak near the pool size with failures. |
| `[CRASH] mem heap total ... commit used ... of ...` | G or crash | heap, commit used | Memory at the crash. |
| `audio cost avg ... us max ... us per 60 Hz tick` | B, C, D, F | average and max us | Audio cost against the 5 ms per tick rule (AU1 section 7). |
| `pace: wall ... steps/s ...` | B, C, D, F | steps per second, intervals 1:2:3:4+ | Speed against 30 fps (target 30.00, PC01). Intervals 3 or 4+ count slow steps. |
| `phase W ...` and `pc` lines | E | wall avg/p95/max ms, cpu ms, over34 and over33 counts | Where core 0 time goes. Needed by PO2, PO3, AU1e, buffer bridge B7. |
| `logstat frames ... work ... ms` | B, C | log work ms per frame, writer core, dropped | Log cost and whether the core 1 writer works. |
| `log: APT_SetAppCpuTimeLimit(30) = ` | A | result code (0 = ok) | Core 1 usable on the O3DS (AU1 option A). Non-zero = core 1 may not be available. |
| `threads: game on core`, `log writer on core` | A | core ids | Core layout. |
| `dspfirm.cdc found` and no `ndsp init failed` | A | found or MISSING | Audio numbers valid only if found. |
| `audio posts ... peak` | B, C | posts, peak level | Sound reaches ndsp. Peak 0 = silent. |

### What each number decides

| Number | Threshold | Decision |
|---|---|---|
| `mem commit limit` | 65,536 KB = assumption holds. Lower = fix first. | Below 65,536: take options D and E of MEMORY_BUDGET section 6 (D 32 KB, E 2,048 KB). Above: more spare, Q7 gets easier. |
| Heap total minus 27,543 KB | Spare with 271 maps. EST 6,657 KB | Compare with the 500 KB reserve. |
| Spare minus 1,988 KB (154 maps) | EST 4,669 KB for all 421 maps | At or above 500 KB: link all 421 maps (S30b, settles Q7). Below: a Q7 question to the user. |
| Spare if the 4 MB N64 block is extra | EST 573 KB | The spare after the 154 maps would be near the reserve. Measure the real value from the heap total. |
| `audio cost avg` per tick | at or below 5,000 us | AU1 option A at limit 30. |
| | 5,000 to 13,000 us | Option A with higher limit (50, 80), then A+. |
| | above 13,000 us | A+ first. Option B research only if A+ fails. |
| `audio cost max` | about 30,000 us in Azahar (likely a ROM load) | Spikes above 33 ms stall a step. Note the scene. |
| `steps/s` | 30.0 = full speed | Below 29: the O3DS misses 30 fps. Use phase run E for the split. PO2 and PO3 gates (PO3 section 9: start PO3b only if GBI interpret still large after PO2). |
| `phase` gbi ms | Azahar 11.1 ms (mac_00), 15.3 ms (kmr_20) | PO3 go or no-go. Share of the 33.3 ms step. |
| `phase` fbegin ms | large share = GPU or vblank wait | GPU is the limit, not the CPU. PO3 section 8. |
| `mem` linear peak, failures | texture cache peak 1,209 KB in Azahar | Failures above 0 on hardware = raise the texture budget or cut it (option E) by data. Pool margin 2,712 KB is the fragmentation reserve. |
| VRAM use | Not logged today. VRAM pool is 6 MB (buffer_bridge section 10). | Not measured in S28 (section 6). |
| `APT_SetAppCpuTimeLimit` result | 0 | Core 1 audio thread is possible (AU1d). |
| N3DS vs O3DS lines | same scenes | Comparison only. The O3DS is the target. |

## 5. Known risks on first boot

| Risk | Sign | Action |
|---|---|---|
| Crash at boot | Luma red or black exception screen, or the game closes | Pull with `pull-o`. Read `[CRASH]` lines in `hw_o3ds_log.txt` (`grep -n CRASH`). Then look at `build3ds/hw_o3ds_dumps/*.dmp`. |
| Out of memory | `[CRASH]` mem line, `commit used` near `of`, or a failed alloc line at boot | Report the `mem commit limit` line. Do not change code in S28. |
| Black or white screen, log runs | Log lines continue, `pace` shows steps | The GPU path differs from Azahar (PO3 risk R4). Take one photo. Check `mem ... texture failures`. |
| Black screen, log stops | No new lines, no crash | A hang. Note the last 40 lines (`tail`). Pull after a power cycle: `log.txt` has the last flushed lines. |
| No sound | `dspfirm.cdc MISSING` or `ndsp init failed rc=` | Put `dspfirm.cdc` on the SD card. Audio numbers are then not valid. |
| ROM missing | Bottom screen message, game stops | Copy `papermario.z64` to `sdmc:/3ds/PaperMario/`. |
| Slow boot | Boot takes long | The 27 MB ROM read from SD is untimed. Time it with a stopwatch from the start to the logos. |
| Netload fails | "no 3DS netloader found" | Press Y in Homebrew Launcher again. `go.ps1` retries 3 times. |
| Force kill | Never kill a started netload | CODE comment in push_3ds.sh: a forced kill left both consoles in corrupt code. |
| Log not flushed after a crash | `log.txt` missing the last lines | The live echo file `hw_live_log_<ip>.txt` is the primary source. |

### How to read a Luma crash dump

1. Skill `pull-o` copies `sdmc:/luma/dumps/arm11/*.dmp` to `build3ds/hw_o3ds_dumps/`.
2. The dump holds registers and the stack. The PC and LR values are the useful part.
3. The log has the `[CRASH]` text from the game handler (pm_crash) first. Use its PC if it exists.
4. Convert a PC to a function with addr2line in Docker. The command exists in `3ds/run_azahar.ps1` line 337: `arm-none-eabi-addr2line -f -C -p -e /work/build/pm_3ds.elf <addr>` in the image `devkitpro/devkitarm:latest` with volume `pm3ds-work`. The ELF in the volume must be the one that was pushed. Do not build between the push and the addr2line. If `push-o` built again, the ELF matches the last push.
5. Luma dump addresses are in the process image. The 3dsx loader relocation can shift them. UNV: check the offset against a known function on the first dump.

## 6. What stays unmeasured by S28

| Item | Why |
|---|---|
| VRAM use | No VRAM line in the log. Needs a log line first (buffer_bridge B7). |
| Real peak after all 421 maps | S28 runs 271 maps. A hardware tour comes later. |
| Fragmentation over a long run | Runs are 60 to 90 s. Allocator state after many map changes needs a long run. |
| Whether the 4 MB N64 block is extra | The heap total shows it only by comparison with the estimate. Not proven in code. |
| Libctru and OS overhead | Not itemised. Only the sum shows in the heap total. |
| Audio ear check | Only the user can hear crackle. Say it in the report. |
| `GX_TextureCopy`, `C3D_FrameSplit`, VRAM read speed (buffer bridge) | Needs B-slices in the build (buffer_bridge section 10). |
| PICA vertex cost, clipper differences (PO3) | Needs PO3 code. |
| `audio wait`, job wall time, underruns | AU1a to AU1d are not built. S28 gives only the lockstep baseline. |
| Battle, pause, file select speed on hardware | Run F covers one battle. The rest is untested. |
| N3DS memory limit for the O3DS | Not valid for the O3DS. |
| Behaviour of `-Seconds` runs of more than 90 s | Not planned. |

## 7. Record of results (fill in after the run)

Write the numbers in HANDOFF.md (at most 3 lines), docs/HISTORY.md (the table), and answer Q2 and Q7 in 3ds/QUESTIONS.md. Mark S28 done in 3ds/SLICES.md and move it to docs/SLICES_DONE.md. This is the S28 agent task. Use haiku for the run and the table. Relaunch on sonnet if the result is unclear.

| Item | O3DS | N3DS |
|---|---|---|
| commit limit KB | | |
| used-by-image KB | | |
| heap total KB | | |
| linear peak KB, texture failures | | |
| audio avg / max us (title, toad, battle) | | |
| steps/s (title, toad, yard, battle) | | |
| phase: wall, cpu, gbi, fbegin ms (toad) | | |
| core 1 time-limit result | | |
| crashes | | |
