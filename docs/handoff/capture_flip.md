# capture_flip: one console prof capture of the file select flip and the pause pane flip (designer: bug oracle 39)
Push (real command, go.ps1:176-185, push_3ds.sh:88-99, :386, :396): `powershell -NoProfile -File 3ds/go.ps1 push o -ip 192.168.1.131 -Seconds 240 -Profile prof`.
3dslink gets `--verbose livelog prof=30 livelevel=err disklevel=off conlevel=off` (bench_defs.json:6). No warp word: the file select is the subject.
Link 1 switch: `prof=30` turns prof mode on (pm_log.c:283, :669-670) and samples 1 frame of 30 (pm_phase.c:811). Launch words win over options.ini (pm_options.c:276-283).
Link 2 sink: a prof line goes to the live socket and the console, not to log.txt (pm_log.c:1001-1002, prof_disk 0).
Link 3 tool: push_3ds.sh echoes the live log to build3ds/hw_live_log_131.txt (push_3ds.sh:29). Backup after the run, game still open:
  `python 3ds/tools/pm_logrecv.py --ip 192.168.1.131 --label xl --out build3ds` writes the replay ring to build3ds/hw_xl_live.txt (pm_log.c:63).
Link 4 proof lines, one set per 300 drawn frames (pm_phase.c:757): `prof f<N> w300 at <map> <area>:<map> m<mode>` (:547; m14 file select, m10 pause,
  src/game_modes.h:28,32), `prof f<N> w300 fps <x> | ft <ms> max <ms> o34 <n> wk <ms>` (:592), `prof f<N> w300 ph gbi .. gpu <ms> | ...` (:597).
FACT (code read): prof prints no per-frame line. ft, max, o34 (frames over 34 ms, :360) and wk (CPU work) cover every drawn frame of the window (:732-747).
  gbi and gpu come from the sampled frames only (1 of 30). A flip of about 10 frames can fall between 2 samples: the gpu value of a flip is OPEN.
  A game mode change drops the window (pm_map.c:375, pm_phase.c:70, :521): stay in one screen for each part.
Ask the user (XL, about 3 min): 1. Title, then the file select. Wait 25 s with no input. 2. Do the file select flip that lags, once every 2 s, for 25 s.
  3. Load the file. In the field press START. Wait 25 s in the pause menu. 4. Change the pause pane once every 2 s for 25 s. 5. Close the menu.
GUESS: 25 s holds 2 full windows at 24 fps or more (300 frames = 10 s at 30 fps). Read: calm window against flip window of the same m<mode>.
  wk up = CPU cost. ft and max up with wk flat = GPU or wait cost. Both flat = no flip cost in the frame time.
