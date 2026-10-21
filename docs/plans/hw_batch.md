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
sh 3ds/push_3ds.sh --no-build --ip $O -t 20 loglevel=debug audiostat texstat   # build 913; then pull log.txt by FTP. The live log is optional.
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

## 4. Push 3 (build 913): one full cycle that rotates the A/B states, pulled log only

Build 913 is main 56e654a2d without map_restore_table.c. The audio path is the old one (AU2 not merged). The file is 3ds/logs/builds/pm_3ds_913.3dsx (copy it to build3ds/pm_3ds.3dsx before the push). The live log is optional: the pulled log.txt is the data. It has no dropped lines (queue 1024 slots, console skipped under backlog). Check `grep -c "lines dropped" <pull>` is 0.

Command (full cycle, 420 s; Push 2 uses the same form with its own switches and `-t 20`):
```
export O=<ip> B=913; sh 3ds/push_3ds.sh --no-build --ip $O -t 20 audiostat pacestat logstat phase phsum noaudit loglevel=debug fixedseed statehash=10 phasecsv=300:400 abtog=base,gpuvtx,nodr1,nokick,notx1,phasegbi:30
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
