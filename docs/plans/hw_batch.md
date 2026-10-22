# Hardware batch plan (O3DS): one build, at most three pushes

Written by HW-FIX. Console IP: use the address that the orchestrator uses (O=192.168.1.168 in the push-o skill, 192.168.1.180 in commands_hw_891.txt). Check it before the first push.

## 1. The one build

Contents (all committed on main):
- HW1 live log: socket buffer at 0x08000000. Log line `live log: port 17492 (buffer ..., alloc 00000000, socInit 00000000)`. A failure prints `alloc` and `socInit` Result codes.
- HW2 save: one write on core 1. Log line `save.flash written: off N len N in N ms (reason)`. Reasons: `core 1 idle`, `HOME menu opens`, `sleep`, `exit hook`, `exit`.
- HW3 ROM read path and texture invalidate. Step timing is in the `pc` lines (phase phasecsv).
- HW5 ELF of the build: 3ds/logs/builds/pm_3ds_<build>.elf. Decode a dump: `python3 3ds/tools/decode_dump.py <dump> --elf 3ds/logs/builds/pm_3ds_<build>.elf`.
- AU1f mixer. New log line `audio core probe` (switch audiostat or coreprobe): shortest, average and longest time of a fixed loop on the audio thread. Average over shortest above 1.0 means core 1 gives the audio thread less than a whole core.
- TC1 texture cache statistics: lines `texstat` every 300 drawn frames (switch texstat or loglevel=debug).
- UI-1 (lane-b commit 3aa8abf4f): Options tabs, D-pad modes, padscript tokens k_dup k_ddown k_dleft k_dright k_L k_R. Merged as 2773574de. The auto mode classifier denied the merge command in the HW-FIX session. The orchestrator or the user must run `git merge lane-b` on main (clean merge expected, lane-b does not touch map_restore_table.c). After the merge: build, run one full Azahar cycle without input, compare triout, vtxslot, draws, texmiss, texKB and statehash f1800 5a6bb12a, f4000 bcd4e3ea, f8000 1325de7e. Report the 3dsx size change.

Build command (excludes the user's uncommitted map_restore_table.c):
```
git stash push 3ds/src/map_restore_table.c && sh 3ds/build.sh; git stash pop
```
Use `--no-build` for every push. Note the build number from build3ds/pm_3ds.3dsx (the `build NNN` line) as B.

User actions before EVERY push: open the Homebrew Launcher, press Y (netload), wait for the orchestrator. After the run press HOME, close the game.

## 2. Push 1: full cycle, 330 s (FTP pull)

Command:
```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 330 audiostat pacestat logstat phase phasecsv loglevel=debug fixedseed statehash=10
cp build3ds/hw_live_log_*.txt 3ds/logs/perf/hw_${B}_P1_live.txt
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_P1_pull.txt
```
User hands: none during the run, except start the FTP app after the run (port 5000). Time: about 6 minutes.

Read:
- Live log works: `grep -E "live log|livelog" 3ds/logs/perf/hw_${B}_P1_pull.txt` must show `live log: port 17492` and the live file must start with `#0`. Compare: `wc -l` of live and pulled file, and `grep -c "GAP" <live file>`.
- Table: `python3 3ds/tools/hw_seg.py 3ds/logs/perf/hw_${B}_P1_pull.txt "build $B O3DS P1" > 3ds/logs/perf/hw_${B}_P1_table.txt`.
- Intro NPC spawn step times: `awk '$2=="pc" && ($3==331 || $3==332 || $3==337 || $3==3484) {print $3,"wall",$5,"ro",$21}' <pull>` (values in 0.1 ms). Azahar after HW3: step 331 wall 985, step 3484 wall 918. Before HW3 on Azahar: 1904 and 4034.
- Audio: `grep -E "audio cost avg|audio wait|audio core probe" <pull> | tail -20`. O3DS before AU1f: cost 4.4 to 11 ms per tick, wait 6.7 to 16.7 ms.
- Texture cache: `grep "texstat" <pull> | awk 'NR%5==1' | head -30` and `grep "texstat f" <pull> | grep -v "evict 0 "`.
- Game state: `grep -E "statehash f(1800|4000|8000) " <pull>` must be 5a6bb12a, bcd4e3ea, 1325de7e.
- Late steps: `grep "pace: wall" <pull> | tail -5`.
- Memory: `grep -E "mem split|mem map heap" <pull> | tail -3`. The socket buffer must not shrink the heap further: heap 32032 KB, linear 14208 KB as in build 891.
- Crash dump check after the run: `ls` the dump folder on the SD card in the FTP app. A new dump: decode with decode_dump.py and the ELF of B.

## 3. Push 2: interactive session with the live log (about 8 minutes)

Command:
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 loglevel=debug audiostat texstat   # build 926; then pull log.txt by FTP. The live log is optional.
```
Build 902 (main 2773574de + HW-FIX, without map_restore_table.c) has the UI-1 work. The live log is the read channel. FTP only if lines are missing.

User hands, minute list:
- 0:00 to 0:40 boot, title. Wait for the title screen.
- 0:40 File select. Start a new game or load slot 1.
- 1:00 File select: move by D-pad. Pause menu (START): move by D-pad through every entry. Open Options. Open each tab (General, Controls, Debug) with L and R.
- 1:45 Controls tab: the D-PAD mode row shows two modes on the O3DS (Multifunction, C-Stick; no Analog). Set each and check it. Rebind one control and check that the new binding works.
- 2:15 Overworld: the D-pad moves the camera (C-stick mode and Multifunction), it does not walk Mario.
- 2:30 Save: save the game in a slot (save block). Wait 3 seconds. Read in the live log: `save.flash written ... (core 1 idle)`. Save a second time in the same slot. The game must not stall.
- 3:30 Enter a battle (any enemy). Navigate the battle menu by D-pad (Fight, Items, Strategies).
- 5:00 Press HOME. Wait 5 seconds. Return to the game (tap the game icon).
- 5:30 Close the lid for 10 seconds (sleep), open it, wake the console. Check that the game goes on.
- 6:30 Save once more, press HOME at once (within 1 second after the save text). The log must show a write with reason `HOME menu opens` or `core 1 idle`.
- 7:30 Press HOME, close the game.

Read:
- `grep -E "save.flash written" <live>`: each save must show 1 line. Time in ms is the SD cost on core 1.
- `grep -E "cpu limit" <live>` : HOME and sleep lines with result 00000000. After sleep or HOME, missing lines are normal in the live file: use FTP pull for `log.txt`.
- `grep -E "FAIL|CRASH|BOOT FAIL" <live>`: must be empty.
- Step times during the save: `grep "pace: wall" <live>` is not available without phase. Judge the hitch by hand: the user reports if the game froze at the save.
- New dump after HOME or sleep: FTP pull of the dump folder, then decode_dump.py with the ELF of B.

## 4. Push 3 (build 926): one full cycle that rotates the A/B states, pulled log only

Build 926 is main 725afa478 (G1 rows and AU2 merged) without map_restore_table.c. Audio now has `audio2:` lines (jobs, ring fill, render busy, drains), and a finished voice is freed up to 2 frames later (stop records). The file is 3ds/logs/builds/pm_3ds_926.3dsx (copy it to build3ds/pm_3ds.3dsx before the push). The live log is optional: the pulled log.txt is the data. It has no dropped lines (queue 1024 slots, console skipped under backlog). Check `grep -c "lines dropped" <pull>` is 0.

Command (full cycle, 420 s; Push 2 uses the same form with its own switches and `-t 20`):
```
export O=<ip> B=926; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,gpuvtx,nodr1,nokick,notx1,phasegbi:30
```
Wait 7 minutes (the game runs the intro cycle). Then start the FTP app and pull:
```
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_P3_pull.txt
python3 3ds/tools/phs_report.py 3ds/logs/perf/hw_${B}_P3_pull.txt --hw > 3ds/logs/perf/hw_${B}_P3_table.txt
python3 3ds/tools/phs_report.py 3ds/logs/perf/hw_${B}_P3_pull.txt --az 3ds/logs/perf/hwm1_A.txt --hw > 3ds/logs/perf/hw_${B}_P3_ratio.txt
```
One state is active for 30 drawn steps (the first 2 are not summed), then the next. Order: base, gpuvtx, nodr1, nokick, notx1, phasegbi. Every scene gets a phs line for each state. Do not press a button during the run (a press changes the scene order and the statehash). Expected statehash f1800 5a6bb12a only if the first intro is not skipped.

Lines to grep in the pull (all at debug level):
- `grep -E "^#[0-9]+ (phs|phh|phc) " <pull>` per scene and state. `grep "abtog " <pull>` shows each switch change.
- `grep -E "^#[0-9]+ pc 33[0-9] " <pull>` the spawn steps 331, 332, 337 (pc lines 300 to 400 only). `grep "pcx " <pull>` has fbread, g_tev, g_bind, g_state, GPU time (us) and command buffer fill per mille for the same steps.
- `grep -E "tx1 f|tx1check" <pull>`, `grep "dr1 f" <pull>`, `grep "fs1 skipped" <pull>`, `grep "logstat2" <pull>`, `grep "statehash" <pull>`.

How to read each gain in phs_report.py output (columns are ms per drawn step, cpu50 and cpu95 in ms, ok% = share of steps with wall up to 34.5 ms):
- Base state: the new reference. Compare with the ratio file (Azahar phs log hwm1_A.txt): the ratios of part 1.3 of docs/plans/hw_rerank.md.
- TX1: tex of steps 331, 332, 337 in the pc lines. Goal: step 332 under 25 ms (was 82.1), step 337 under 35 ms (was 113.3). State notx1 gives the old path in the same run: tex column of notx1 minus base. `tx1 f<frame>` shows ns per byte (old path 154 in Azahar, new 41).
- DR1: gp + gd divided by the draw count of the state (draws column) gives microseconds per draw. Goal under 45 us (was 63). State nodr1 against base shows the gain. `dr1 f` shows merged rectangles and uniform skips.
- KICK1: state nokick against base. Read cpu50, ok% (share at cpu 29 to 31 ms: phh line bins ok and late) and the GPU wait: the fb and fe columns and the gpu field of the phc line (citro3d GPU time in us per drawn step, sum over the window, divide by n). Goal: ok share above 60 percent at cpu 29 to 31 ms (was 12 to 39).
- FS1: `fs1 skipped N of M steps` in base state. Skips should be few. Read `pace:` lines for steps per second.
- GV1 inputs: state gpuvtx (gb and gd against base) and state phasegbi (gv, gt, gm, go columns; the report subtracts the tick read cost of gb when --hw is given).
- Log health: `logstat2` max us file, sock and console per 300 frames, console skipped count, queue fill. Dropped lines must be 0.

## 5. Push 3 old (only if Push 1 shows audio wait above 3 ms or core probe average over shortest above 1.5)

Same scene as H4 in commands_hw_891.txt, three runs of 60 s: cpulimit=30, 50, 80 with `audiostat pacestat logstat phase loglevel=debug preset=toad`. Read: `grep -E "audio cost avg|audio wait|audio core probe|pace: wall|cpu limit: set"`. User hands: none.

## 6. What cannot be known before the push

- The real O3DS gain of AU1f (Azahar: audio cost per tick 4606 to 3317 us).
- Whether `socInit` and the accept path work on the console (Azahar: yes, buffer at 0x08000000).
- The ms of a save on the SD card of the console.
- The HOME and sleep behaviour of the flash thread (tested only with the switch `flashfake` in Azahar).

## 7. Push BENCH: micro-benchmarks on the console (pulled log only)

Build: the file is 3ds/logs/builds/pm_3ds_bench_<build>.3dsx in the main folder (lane C, branch lane-c, without the uncommitted map_restore_table.c change). Copy it to build3ds/pm_3ds.3dsx before the push. The benches run at the start of the frame loop, after the GPU and audio init, and end the game with `benchexit`. Time: about 30 to 60 s on the console. User hands: none, except start the FTP app after the run.

Command:
```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 bench benchexit loglevel=debug
```
Wait 90 s. Start the FTP app (port 5000). Then pull:
```
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_BENCH_pull.txt
```
Optional second push, only after the first one ended well: add `bench=r2` (GPU call of a sub command buffer) or `bench=svcflush` (SVC may end the process: the last line is then `bench: svcflush start`). Both are not in `bench`.

Lines to grep in the pull:
- `grep "bench:" <pull>` all results. Draw ladder: `grep "bench: draw" <pull>`. Flush: `grep "bench: flush" <pull>`. Texture and writes: `grep -E "bench: (tex|wr) " <pull>`. Replay: `grep "bench: replay" <pull>`. Frame wait: `grep "bench: fbwait" <pull>`.
- `grep -E "bench: (begin|end)" <pull>` must show `console O3DS` and a total time.
- How to read each line and the decision rules: docs/plans/bench_notes.md.

## 8. Build 970 (main d54053ce5 without map_restore_table.c): the new defaults and four pushes

### 8.0 Two pushes for build 1011: PLAY and SAVE (the user plays by hand)
Build 1015 fixes the O3DS boot crash (ROM did not fit the RAM cap by 38 KB: ROMFIT). Push PLAY as before. Use 3ds/logs/builds/pm_3ds_1015.3dsx and `B=1015` for the commands below (the text says 1011). Check in the pull: the boot line `rom resident ... cap ... margin` and no `ROM does not fit` line.
Build 1011 (main 702308b3f with LB-3 and VIS-4, without map_restore_table.c). No automated test runs on the console. Azahar gate for build 1011: the full cycle boots through the intro (no `warp=` at boot since LB-3). Statehash references for this start: f1800 5a6bb12a, f4000 bcd4e3ea, f8000 1325de7e. The set 53090ae9, d95fd1a4, 82add753 belongs to the old `warp=osr` start (intro skipped). Do not mix the two sets. Host tests dspdec and au2 ok. No golden run for this build.
Options: the game now reads `options.ini`. An old `options.txt` on the card is migrated. The `warp=` line is ignored. The user can delete it.
Common start:
```
cp 3ds/logs/builds/pm_3ds_1011.3dsx build3ds/pm_3ds.3dsx
export O=<ip> B=1011
```
#### Push PLAY (speed and DLC data)
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dspaudio audiostat texstat pacestat logstat phase phsum abtog=base,dlc:60 loglevel=debug
```
`abtog` switches the display-list cache `dlc` on and off every 60 drawn steps. Both states draw the same picture. The user plays as long as they want, then HOME or the Select menu row Exit to Homebrew. Start the FTP app (port 5000) and pull:
```
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_PLAY_pull.txt
```
Read-out (P = the pull file):
- `grep "abtog " P`: each state change. `grep -E "^#[0-9]+ (phs|phh|phc) " P`: phase averages per scene and per state (base against dlc: gb, gd, gp columns). `grep " dlc" P`: the dlc counters.
- `grep "pace:" P`, `grep " spike " P`, `grep texstat P`.
- `grep -E "dspaudio: (first aux callback ran|aux mask)|audio dsp:|audio2:|audio cost" P`.
- `grep -E "FAIL|CRASH|lines dropped" P` must be empty or 0. `grep "exit:" P` shows a clean shutdown.
The HUD may not show the abtog state: use the `abtog` lines of the log with their frame numbers.
Audio fallbacks: add `dspfx=off`, then `dspgain=50 dspflush`, or drop `dspaudio`.
#### Push SAVE (save block faults, pipe faults, Merlon door)
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dspaudio audiostat pacestat logstat phase phsum camlog entlog mtxlog nogv1 noraw1 nofl1 fs1off loglevel=debug
```
This push turns off GV1, RAW1, FL1 and FS1. The user plays the Toad Town save block scenes, the Yoshi village save block and the Merlon door. If the faults vanish with these switches, they come from GV1, RAW1, FL1 or FS1: then push again with one switch dropped at a time (`nogv1`, `noraw1`, `nofl1`, `fs1off`) to find the one. `camlog` writes up to 60 lines per second and `mtxlog` at most 50 lines per 30 frames: the log gets long, so keep the session short and note the minute of each fault. Pull as for PLAY (copy to `hw_${B}_SAVE_pull.txt`). Read-out:
- `grep -i camlog P | head`, `grep -i entlog P`, `grep -i mtxlog P`: camera, entity and matrix lines around the noted minute.
- `grep "pace:" P`, `grep -E "FAIL|CRASH|lines dropped" P`, `grep "exit:" P`.

#### Push CRASHTEST (optional, only if the user asks)
Tests the crash handler (`svcExitProcess`) and the watchdog (`wdog`). Details and expected lines: docs/plans/crash_exit.md.
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 crashtest=1 loglevel=debug
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 crashtest=2 wdog=5 loglevel=debug
```
Check: after `crashtest=1` the game must leave to the Homebrew menu (no frozen screen, no reboot) and the log must end with the crash handler lines. After `crashtest=2 wdog=5` the log must show `watchdog: no progress for 5 s ...` and `watchdog: exiting the process`, and the console must return to the menu. Pull both logs with ftppull.py.

### 8.0b Optional pushes, not planned (the user chose hand play)
Push R (route), A (full cycle abtog), B (dspaudio listening) and C (bench, dsptest) stay for later use. They are in sections 8.1 to 8.4. Each needs the run time and hands listed there. Section 8.4 explains `play1_nosave` and the route file upload.

File: 3ds/logs/builds/pm_3ds_970.3dsx (build 967 files stay in the same folder; the text below was written for build 967 and is valid for 970) (ELF and map next to it). Copy it to build3ds/pm_3ds.3dsx before each push. The orchestrator pushes. Defaults in this build: the full GPU path gv1, the lean draw path raw1 (one DVLB, lazy program and buffer binding, uniform shadows) and the texture slab fl1 (one data cache flush per command list submit) are ON. Switches `nogv1`, `noraw1`, `nofl1` turn each off. `dspaudio` and `bench` are OFF unless the switch is set. Lane B (AUD-DSP) and lane C (BENCH) are merged.
New statehash references (Azahar, fixedseed statehash=10 cpulimit=80, equal with the three paths on and off): f1800 53090ae9, f4000 d95fd1a4, f8000 82add753. The old values 5a6bb12a, bcd4e3ea, 1325de7e are from before PT07. Do not use them.
Azahar numbers (dip window pc 1702 to 1850, noaudit): gb 13.20 (all three off) to 11.05 (on), gd 2.01 to 2.08, gp 1.21 to 1.24, draws 150.6 to 152.4, triout 290 to 389, citro3d calls per raw draw 10 to 2.2. Whole cycle 8233 steps: gbi 8.32 to 6.64 ms per step, draws 89.8 to 91.8 per step, statehash equal.

### 8.1 Push A: full cycle that rotates gv1, raw1 and fl1 (420 s, pulled log only) (optional, not planned)
```
export O=<ip> B=967; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,nogv1,noraw1,nofl1:30
```
Wait 7 minutes. Start the FTP app. Pull and make the tables as in section 4 (phs_report.py with --hw). State base = all three on, nogv1 = full GPU path off (and so raw1 has no raw draws), noraw1 = only the lean binding off, nofl1 = only the slab flush off. Do not press a button.
Read:
- `grep -E "^#[0-9]+ (phs|phh|phc) " <pull>`: gb (gbi), gp, gd, gc columns per state. Gate for gv1: base gb lower than nogv1 gb by 6 ms or more at the O3DS (Azahar says 2 ms, weighted 4 to 7 ms).
- phc line, last counters: flushcalls and flushkb (all GSPGPU_FlushDataCache calls per step and KB), rawd (raw draws per step), rawcall (citro3d calls in the raw draw path). Expected: rawcall about 2.2 per rawd with raw1, about 10 without. flushcalls: 4 or fewer per step in a texture spawn step with fl1, up to 120 without.
- `grep -E "^#[0-9]+ pc 33[0-9] " <pull>` and `grep "pcx " <pull>`: the spawn steps 331, 332, 337. The pcx line now ends with flushcalls and flushkb. Goal with fl1: tex ms per miss under 0.4 ms (was 1.21), step 332 under 40 ms.
- gd per draw in the raw1 state against noraw1 (the GPU command part), and gpu time (us) in the phc line.
- `grep statehash <pull>`: f1800 53090ae9, f4000 d95fd1a4, f8000 82add753.
- `grep -c "lines dropped" <pull>` is 0. `grep -E "FAIL|CRASH" <pull>` is empty.

### 8.2 Push B: DSP audio, interactive listening (optional, not planned)
```
export O=<ip> B=967; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dspaudio audiostat loglevel=debug
```
Play the A/B list of docs/plans/AUD_DSP.md section 15 with the live log or the pulled log. Then push once without `dspaudio` for the second half of each comparison. Pull and read `audio dsp:`, `dspaudio:`, `audio cost` and `audio2:` lines.

### 8.3 Push C: BENCH and the DSP test (optional, not planned)
```
export O=<ip> B=967; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 bench benchexit loglevel=debug
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dsptest loglevel=debug
```
Read as in section 7 (bench lines, decision rules in docs/plans/bench_notes.md). The second push gives the `dsptest:` lines (control call costs, aux callback core).

### 8.4 Push R: route play1, play-through benchmark on the console (pulled log only, no hands) (optional, not planned)
Build: 970 or later (ROUTE is merged in main, file `pm_route.c`). Route file: `3ds/routes/play1.txt`. One time per route change, put it on the SD card (FTP app on the console, port 5000):
```
export O=<ip>; curl -T 3ds/routes/play1.txt --ftp-create-dirs ftp://$O:5000/3ds/PaperMario/routes/play1.txt
```
The game reads `sdmc:/3ds/PaperMario/routes/play1.txt` (`route=play1`, same as `padscript=@routes/play1.txt`). WARNING: `play1` saves to the real save slot 3 (`save_3`) on the console. To avoid it, use `play1_nosave` instead (same route without `save_3`, same run hash 2440c7c2 in Azahar): put `3ds/routes/play1_nosave.txt` on the SD card with the same curl command and set `route=play1_nosave`. The report then shows no `save.flash` line. If you use `play1`, copy the save files from the SD card first if slot 3 holds data. User hands: none, except start the FTP app before the curl and again for the pull.
Push (short live log, the route runs on after the log ends):
```
export O=<ip> B=<build>; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 warp=mac_00:4 story=ch0_toad_town partner=1 route=play1 phase phsum phasecsv loglevel=debug fixedseed statehash=300
```
Wait 150 s (boot 10 s, route 91 s at 30 steps per second, slower steps make it longer, the end line is `route: end`). Start the FTP app. Pull and report:
```
python3 -u 3ds/tools/ftppull.py $O o build3ds; cp build3ds/hw_o3ds_log.txt 3ds/logs/perf/hw_${B}_ROUTE_pull.txt
python3 3ds/tools/route_report.py 3ds/logs/perf/hw_${B}_ROUTE_pull.txt --title "ROUTE play1 O3DS $B" > 3ds/logs/perf/route_hw_${B}.txt
python3 3ds/tools/route_report.py 3ds/logs/perf/hw_${B}_ROUTE_pull.txt --compare 3ds/logs/perf/hw_<other build or run>_ROUTE_pull.txt
grep -E "route: (seg|goto|save|leg|end)|save.flash" 3ds/logs/perf/hw_${B}_ROUTE_pull.txt
```
Read: the `wall36` column (late steps the player sees), `wall_p95`, the `save` segment and the `save.flash written ... ms` line (the save hitch), the map entries and the battle segments. The seg lines of the console must match Azahar within 2 steps (3ds/logs/perf/route_ref_*.txt). If `route: leg blocked` appears, the route left its path: tell the ROUTE agent. Notes: docs/plans/route_notes.md.

### 8.0e Push PLAY2 (DLC1 and DLC2 against base, lane B build)
Build the lane B tip with `PM_LANE=laneB sh 3ds/build.sh` (3dsx in build3ds/lane-laneB/), copy it to build3ds/pm_3ds.3dsx, set `B=<build number>` and `export O=<ip>` as in 8.0d. DLC stays off by default. The switch below turns it on per state.
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dspaudio audiostat texstat pacestat logstat phase phsum abtog=base,dlc1,dlc:60 loglevel=debug
```
`abtog` rotates three states every 60 drawn steps: `base` (no dlc), `dlc1` (display-list compile and box skip, vertices through the ring) and `dlc` (DLC1 plus the static vertex area, DLC2). All states draw the same picture. Play in Toad Town (mac_00, mac_01) for at least 3 minutes so each state has 300 or more steps per scene, then HOME or the Select menu row Exit to Homebrew. Start the FTP app and pull as in 8.0d (`hw_${B}_PLAY2_pull.txt`).
Read-out: `grep "abtog " P`, then `python3 3ds/tools/phs_report.py P` and compare the rows `base`, `dlc1`, `dlc` of the town scene (columns gb, gd, gp, cpu50). `grep -E " dlc2? f" P` gives the counters (`static grp` must be close to `repl`, `fail`, `hashbad`, `exitbad`, `missing`, `slotfix` 0).
Gate: gb of `dlc` lower than `base` by 6 ms or more in town (expected from Azahar x3.5: DLC1 about 7 ms, DLC2 a further 2 ms). The CPU part (cpu50) must fall under 50 ms for 20 drawn steps per second. If the gate holds, the orchestrator sets `PM_DLC_DEFAULT` to 1 in a later row. Azahar results (docs/plans/DLC.md section 7): gbi 11.11 (base), 9.10 (dlc1), 8.51 ms (dlc).

### 8.0f Push PLAY3 (PF1 prefetch against none)
Same build and commands as 8.0e with this `abtog` (DLC on in both states through the switch `dlc`):
```
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 dspaudio audiostat texstat pacestat logstat phase phsum dlc abtog=base,nopf:60 loglevel=debug
```
`base` has the prefetch on, `nopf` turns it off. Read the town rows of `phs_report.py` for `base` and `nopf` (columns gb, dl, cpu50). Gate: base lower than nopf by 0.5 ms or more in gb. A smaller difference means the stalls are not memory bound: remove nothing, the prefetch costs under 0.1 ms.
Result of build 1036 (O3DS, log 3ds/logs/perf/hw_1036_PLAY2_o3ds_pull.txt, clean exit, phs_report.py --hw). World mode (4), town mac_01 (scene 1:1), ms per drawn step, n = drawn steps: base n 232: gb 46.59, gd 4.03, gp 4.82, cpu50 76.6. dlc1 n 232: gb 40.71 (gain 5.88), gd 4.15, gp 4.88, cpu50 70.3. dlc n 268: gb 38.58 (gain 8.01), gd 4.24, gp 4.95, cpu50 66.1. Other worlds, gb base to dlc: 23:0 32.57 to 24.25 (gain 8.32, n 45 and 52), 0:7 31.23 to 28.08 (n 88 and 36), 0:5 25.73 to 23.39 (n 44 and 31). Counters: hashbad 0, exitbad 0, missing 0, slotfix 0, ringfail 0, static fail 0 in the three dlc2 lines read (f1799, f2279, f2699). GATE gb: PASS (dlc gb is 8.01 ms lower than base in town, limit 6 ms). cpu50 limit 50 ms: NOT met in town (66.1 ms); it is met in 23:0 (42.1 ms).
