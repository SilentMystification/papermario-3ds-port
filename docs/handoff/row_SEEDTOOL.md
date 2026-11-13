# Row SEEDTOOL: 3ds/tools/seedprobe.py, one verdict line per seed (build, one run, cmp against the stored base, restore)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
Blocked by DRECSTEP (frame lines in drec.txt for a step window).
## Facts (ORACLE8, main of 2026-11-13)
- bench.py: `base <bench>` stores the merge-base shots or drec.txt once (BASECACHE), `run <bench> --row R --why W`, `cmp <bench>
  --row R` prints per file `equal` or a difference and `n/m byte-equal`, exit 0 when all are equal (bench.py:20-37).
- After DRECSTEP a drec window with a step writes `frame <f>` lines in drec.txt. The draws of frame f are the lines from
  `frame f` to the next `frame` line.
- An uncommitted edit keeps the build number. The proof of the rebuild is the sha1 of build3ds/lane-<L>/pm_3ds.elf.
Start: `sh 3ds/tools/wt_new.sh SEEDTOOL`. PM_ROW=SEEDTOOL PM_LANE=SEEDTOOL on every call.
## Touch only (places)
New 3ds/tools/seedprobe.py. New names: none. Host link: none.
## Exact edits
1. seedprobe.py `<bench> <seed> [<seed> ...]`, a seed = `<file>:<line>@<frame>|<old text>|<new text>` (frame: the drec frame the
   seed must change; `-` for a whole-window bench). Run from the worktree root with PM_ROW and PM_LANE set.
   a. `bench.py base <bench>` (stored: no run).
   b. For each seed: in <file> line <line> the old text must occur once; replace it. Else print `seed <file>:<line> bad-spec`
      and exit 2 without a build.
   c. sha1 of build3ds/lane-$PM_LANE/pm_3ds.elf, then `sh 3ds/build.sh` (output to build3ds/out/seedprobe_build.txt), then the
      sha1 again. Equal: print `seedprobe no-rebuild` and exit 2.
   d. `bench.py run <bench> --row $PM_ROW --why "seedprobe"` (output to build3ds/out/seedprobe_run.txt), then
      `bench.py cmp <bench> --row $PM_ROW` (output to build3ds/out/seedprobe_cmp.txt).
   e. `git checkout -- <files>` (restore), always, also after a fault.
   f. Verdict, one line per seed: for `@<frame>`, compare the drec.txt sections of that frame in the run and in the base:
      `seed <file>:<line> live (frame <f> differs)` or `seed <file>:<line> dead (frame <f> equal)`. For `@-`: live when cmp
      shows any difference. A tool fault (no run record, no drec.txt): `seedprobe tool-fault <text>`, exit 3.
   It prints nothing else on stdout.
2. Commit: `SEEDTOOL: seedprobe.py gives one live or dead line per seed`.
## Gate (expected values)
1. On drecstep_89_33 (DRECSTEP, fxtest 89 at frame 412, 33 at 456): seed A =
   `src/effects/water_splash.c:183@412|guTranslateF(sp10, sp94, temp_fp, data->unk_04.z)|guTranslateF(sp10, sp94, temp_fp + 20.0f, data->unk_04.z)`.
   Seed B = `src/effects/music_note.c:149@456|fxData->pos.y|fxData->pos.y` (old equal to new: no change). Expected: A live, B dead.
   This is the seeded set: B is the dead control. Before the probe, `git status` shows the file clean; after, clean again.
2. A spec with old text not on the line: `bad-spec`, exit 2, no build.
3. Budget 8 Azahar starts (the base pair and 1 run). Counter: none moves (tool row).
## Stop and end
A verdict other than the gate, a file left changed, or a tool fault: stop, commit, hand back the exact output. Delete this row
file at the end.
