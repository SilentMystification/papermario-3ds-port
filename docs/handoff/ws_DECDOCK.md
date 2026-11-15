# ws_DECDOCK: decode_dump.py finds addr2line in docker, and reads a crash.txt entry (tool fix, no game code, no counters)
Model: Haiku. PM_ROW, PM_LANE = DECDOCK. Branch DECDOCK, worktree build3ds/wt/DECDOCK. Budget: 0 Azahar starts, 0 builds. Start now.
Owns: 3ds/tools/decode_dump.py, 3ds/tests/host.d/decdump.sh (new), 3ds/tests/decdump/ (new fixtures). Edit no other file.
FACT (decode_dump.py:17-21, :92-103): the tool finds arm-none-eabi-addr2line only on PATH or in /opt/devkitpro (a Linux path), so on Windows it prints "arm-none-eabi-addr2line not found".
FACT (3ds/push_3ds.sh:400-401): the project already runs the devkitARM toolchain as `docker run --rm -v <dir>:/out:ro devkitpro/devkitarm:latest <tool>` (set MSYS_NO_PATHCONV=1 from Git Bash).
FACT (user crash 2026-11-15): crash.txt holds `[CRASH] data abort thread=logic (..) pc=0023f6c8 lr=001ba2a8 ... build #5774` and lines `[CRASH] stack AAAAAAAA BBBBBBBB ...` (code-like words), up to `[CRASH] exiting the process`.
## Group 1 (host test first, commit `DECDOCK 1: ...`)
1. decode_dump.py: new function after find_tool() (the one `a2l` call replaces the 3 subprocess calls):
```
def a2l(elf, addrs):
    """-> [(function, location)] for each address: the local tool, else ONE docker run (devkitpro/devkitarm)."""
    ads = ["0x%x" % a for a in addrs]
    tool = find_tool()
    if tool:
        cmd = [tool, "-f", "-C", "-e", elf] + ads
    else:
        d, n = os.path.split(os.path.abspath(elf))
        cmd = ["docker", "run", "--rm", "-v", d + ":/e:ro", "devkitpro/devkitarm:latest", "arm-none-eabi-addr2line", "-f", "-C", "-e", "/e/" + n] + ads
    r = subprocess.run(cmd, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    L = r.stdout.split("\n")
    return [(L[2 * i], L[2 * i + 1] if 2 * i + 1 < len(L) else "?") for i in range(len(addrs))]
```
Replace the block `tool = find_tool()` ... through the stack-word loop (lines 92-103) by: `res = a2l(elf, [regs["pc"], regs["lr"]] + sw)` where `sw` = the stack words with `0x100000 <= w < 0x440000`, print `pc`, `lr` as before (`"%s %08x -> %s (%s)"`) and then the first 12 stack words whose function is not `??` (`"  stack word %08x -> %s"`). A failed docker run (empty output) prints `addr2line failed: <stderr first line>` once and returns.
2. decode_dump.py: new mode `--crash FILE [--build N] [--elf ELF]`: read FILE, take the LAST block that starts with a line containing `[CRASH] data abort` or `[CRASH] prefetch abort` (or the block with `build #N` when --build is given), up to the line `[CRASH] exiting the process`. Parse `pc=` `lr=` (hex, 8 digits) and the `build #(\d+)`, and every hex word of the lines `[CRASH] stack ...` (skip the `stack used` line). The ELF is `3ds/logs/builds/pm_3ds_<build>*.elf` when it exists (a missing one: print `no ELF for build N: use --elf` and exit 2), else --elf. Print the header line of the block, then the same pc, lr and stack lines as step 1 (one a2l call). Do not run the `check_code` match in this mode (crash.txt has no code bytes): print `(crash.txt: no code bytes: the names are right only if the ELF is the build N)`.
## Group 2 (commit `DECDOCK 2: ...`)
3. Fixtures: 3ds/tests/decdump/crash_a.txt (2 blocks: an old one `build #2050`, a new one with `pc=0023f6c8 lr=001ba2a8 build #5774` and 2 stack lines of 6 words) and 3ds/tests/decdump/a2l_stub.sh (a shell script named arm-none-eabi-addr2line in that folder: for every argument that starts with 0x it prints two lines `fn_<arg>` and `file.c:1`; it ignores the other arguments), a fake ELF file `3ds/tests/decdump/pm_3ds_5774.elf` (any 16 bytes: the tool does not read it in --crash mode).
4. 3ds/tests/host.d/decdump.sh (same style as the other host.d scripts; read gtin.sh for the shape): copy the stub to a temp dir as `arm-none-eabi-addr2line`, put that dir first on PATH, run `python -I 3ds/tools/decode_dump.py --crash 3ds/tests/decdump/crash_a.txt --elf 3ds/tests/decdump/pm_3ds_5774.elf`, and compare to an expected output file 3ds/tests/decdump/expect_a.txt (written by hand from the stub rule: `pc 0023f6c8 -> fn_0x23f6c8 (file.c:1)` and so on: write it from the step 2 print format, run once, check each line by eye). Print `decdump ok`. Seeded fault: a second run with `DECDUMP_SEED=1` makes the test copy decode_dump.py to a temp file with the `build #` regexp changed to `build #x`: the expected compare must FAIL (print `decdump FAIL build`), as the other seeded faults of host.d do.
5. `sh 3ds/tools/nd_host_test.sh`: PASS with `decdump ok`. Real use check (no run budget): `python -I 3ds/tools/decode_dump.py --crash build3ds/hw_o3ds_crash.txt` on Windows with docker running: it prints `reset_player_status` for the pc of the build #5774 block (FACT: the map of that build gave `reset_player_status+0x1b0`). If docker is not running, say so in the hand-back: no failure.
Stop rule: a host FAIL or a docker mount error that you cannot read from the output: 2 tries per named cause, then write build3ds/out/decdock.txt and end your turn with the path. Commit; the commit deletes this page (the INDEX.md line stays: the orchestrator removes it).
Hand-back file first 8 lines: hashes, host result, the real-use line (the function name printed), the docker run time.
