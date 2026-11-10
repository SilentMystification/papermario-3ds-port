# Row TOOLB1: a run is bound to its build (Haiku)

(`sh 3ds/tools/wt_new.sh TOOLB1`, PM_ROW=PM_LANE=TOOLB1. Output to build3ds/out/. Delete this row file in the completing commit.)

## Facts (oracle batch 9)

Already fixed (code read confirms): BASEVERIFY two-run check (bench.py base_run), base lock with dead-pid removal (base_lock_take), base runs exempt from the budget (run_azahar.ps1:80,89, the exit 9 fix).

Still open:
A. No stale-build check. bench.py cmd_run (1122) and run_azahar_list (917) only call lane_problem (894), which tests whether lane-<x>/pm_3ds.3dsx exists. They never compare build_key.txt (written by build.sh:221 from key_now, build.sh:99-107) with the current tree. This is why HC4e2a and HC7 ran on old builds.
B. Wrong build identity in run records. The commit comes from the log's build line (HEAD at build time). The dirty flag comes from git_dirty() at ingest time (bench.py:753). A dirty build that is ingested after its commit is recorded as clean at the old commit (HC4e3 above). Fault A's check removes this: when the key matches, the tree that ran is the tree being ingested.
C. A gdb run loads no ELF. run_azahar.ps1:68/70 mounts the docker volume pm3ds-work[-<lane>], which build.sh never writes (build.sh uses pm3ds-build-<lane> and copies the ELF to the host folder build3ds/lane-<x>/pm_3ds.elf, build.sh:212).
D. Crash symbols are lost. run_azahar.ps1:630 runs addr2line on the same empty volume. Its tool path is "$DEVKITARM/bin/..." inside a PowerShell double-quoted string; $DEVKITARM is an undefined PowerShell variable, so the path becomes "/bin/arm-none-eabi-addr2line". The output goes to a hidden child window that is not captured (azt.ps1:581-582). azt.ps1:764-765 prints "See $runOut", but $runOut is never assigned, so the message ends in a blank path. This is why today's crash came with no symbols.
E. run_id values repeat across worktrees. append_runs (bench.py:304) takes the max id of its own tree's runs.jsonl, under runs.lock in its own tree (bench.py:268). Every worktree hands out the same ids; read_runs then gives a new id to a repeated one after a merge, so a run id quoted in a handback can point to another run.
F. A cold gate runs warm. battle_start_f182_cold says "-NoWarm, a new Azahar process", but "extra": [] (azt_scenarios.json:9477), and run_azahar.ps1:230 decides warm unless -NoWarm is passed. 9 of its 12 HC4e3 runs record warm 1. The other DET scenarios say -NoWarm and pass nothing too. mac01 warm and cold both matched the base, so this is a mislabel there. For the cold DET gate it voids the gate.
G. The 40 min base wait is by design: BASE_LOCK_WAIT_S = 40 min (bench.py:1505), and azt can queue up to 900 s plus 900 s per run. The real cause is rows building the merge-base base lazily and at the same time. Process fix, no code: the orchestrator runs `bench.py base --fill` in the main folder after each merge, so rows find the base already stored.

Shared cause of A-D: a run is not bound to the build that ran. Each tool takes the build identity from a different place: the lane folder exists (A), git at ingest time (B), a docker volume no one writes (C, D). It should come from the lane's own build outputs: build_key.txt, pm_3ds.elf on the host, the boot build line. E has a second cause: state kept per worktree where it must be shared.

## Row TOOLB1 (Haiku): a run is bound to its build
1. 3ds/build.sh: directly after the key_now() function (line 107), add `if [ -n "$PM_KEY_ONLY" ]; then key_now; exit 0; fi`.
2. 3ds/tools/bench.py: add a function build_stale(lane):
   - out = REPO/"build3ds"/("lane-"+lane if lane else "").
   - want = (out/"build_key.txt").read_text().strip(), or "" when missing.
   - now = subprocess.run([shutil.which("sh") or "sh", "3ds/build.sh"], cwd=REPO, env=dict(os.environ, PM_KEY_ONLY="1", PM_LANE=lane), capture_output=True, text=True).stdout.strip().splitlines()[-1].
   - Return "" when want == now, else "STALE BUILD: %s was built from other sources (key %s, tree %s). Run PM_LANE=%s sh 3ds/build.sh, then rerun".
   - Call it in cmd_run (after the lane_problem check, line 1141) and in run_azahar_list (after its lane check). Skip it when os.environ.get("PM_BASE_SNAP") or a.build is not None (base and build-at runs bring their own build). On a stale build print "<reason>. Nothing was run." and return 5.
3. 3ds/run_azahar.ps1:
   - Delete the $vol lines (68, and the $vol part of 70).
   - Line 395-397: drop `-v ${vol}:/work`; use the ELF `/out/pm_3ds.elf`.
   - Line 630 becomes: `& docker run --rm -v "${out}:/elf" devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -C -p -e /elf/pm_3ds.elf @addrs | Where-Object { $_ -notmatch "^\?\?" } | Tee-Object -FilePath "$out\last_crash.txt"`.
   - Line 12: write "the lane ELF build3ds/lane-<x>/pm_3ds.elf" in place of "the pm3ds-work volume".
4. 3ds/tools/azt.ps1:
   - Before line 582 add `$runOut = Join-Path $outDir "last_crash.txt"; Remove-Item $runOut -ErrorAction SilentlyContinue`.
   - Line 764: print `Get-Content $runOut -ErrorAction SilentlyContinue` before the FAILED line.
   - Keep the no-redirect comment (line 581).
Gate (0 Azahar starts):
(a) In a worktree, add one comment line to a tracked .c file without building. `python 3ds/tools/bench.py run spot_mac01_det --row TOOLB1 --why "stale check"` prints STALE BUILD and exits 5; test_budget.csv gets no new start line.
(b) Revert the comment. `PM_KEY_ONLY=1 PM_LANE=<lane> sh 3ds/build.sh` prints the same key as build3ds/lane-<lane>/build_key.txt.
(c) `docker run --rm -v "<abs path>/build3ds/wt/HC4e3/build3ds/lane-HC4e3:/elf" devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -C -p -e /elf/pm_3ds.elf 0x0018edd0` prints a src/model.c line in mdl_get_vertex_count or mdl_project_tex_coords.
(d) Both .ps1 files parse: [System.Management.Automation.Language.Parser]::ParseFile gives 0 errors.
Stop: any gate fails, then hand back its output.
