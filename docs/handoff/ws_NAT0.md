# ws_NAT0 (Haiku): the end proof tool c0gate.py: core 0 runs no N64 state machine. Counters: new C13 (calls), C14 (state), plan_native.md section 2.
Designer: oracle NATIVE. Branch from main. Worktree build3ds/wt/NAT0, branch NAT0. PM_ROW=NAT0 PM_LANE=NAT0. No Azahar run. Edit only: new 3ds/tools/c0gate.py, new 3ds/tools/c0gate_names.txt.
Facts: elfgate.py:71 parse_elf(path) gives (syms, count), sym = (name, value, size, typ, sec), typ 2 FUNC, 1 OBJECT. elfgate.py:41 base_name strips clone suffixes. The ELF of main holds the FUNC nd_stream_run and the 23 state OBJECTs below. Interpreter calls sit in 3ds/src/*.c and src/*.c (src/msg_draw.c:1747). A definition starts at column 0.
Edit 1, c0gate_names.txt: first line `# c0gate names (plan_native.md section 1)`. Then one line `interp <name>` for each of these 45 names:
nd_nr_exec nd_nr_run nd_nr_list nd_list_run nd_ent_units nd_ntab_run nd_range_run nd_scomp_run gt_run nr_set nr_geo nr_ctl nr_unknown ls_run part_units nd_unit_same nd_unit_apply set_om set_rare set_mw set_geom set_scissor set_fogcolor
set_blendcolor set_fillcolor set_primdepth nd_set_prim nd_envc_set nd_comb_set nd_tx_timg nd_tx_tile nd_tx_tsz nd_tx_ldblk nd_tx_ldtile nd_tx_ldtlut nd_tx_texon nd_comb_sync sync_combine_work sync_combine_work_timed scm_replay comb_in_walker nd_rnd_sync box_fold_sync sc_adopt ds_take
Then one line `state <name>` for each of these 23 names: nd_ws conv_k4 keyc keys dual_on box_prod box_folded box_save comb_set comb_alpha_shade comb_gen cmb_gen_seen sc_ok sc_sig fogc blendc sci_x0 sci_y0 sci_x1 sci_y1 fog_fm fog_fo tst_f
Edit 2, c0gate.py (python 3, no pip package): a docstring (usage, the 3 numbers, the column 0 rule), then this logic:
```python
import pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent)); import elfgate
ROOT = pathlib.Path(__file__).resolve().parents[2]; CALL = re.compile(r"\b(\w+)\s*\("); CMT = re.compile(r"/\*.*?\*/", re.S)
def names():
    c = {"interp": set(), "state": set()}
    for ln in (ROOT / "3ds/tools/c0gate_names.txt").read_text(encoding="utf-8").splitlines():
        if ln.strip() and not ln.startswith("#"): k, n = ln.split(); c[k].add(n)
    return c["interp"], c["state"]
def src_calls(files, interp):  # call sites per file, and on a column 0 line the first name is the definition
    per = {}
    for f in files:
        t = f.read_text(encoding="utf-8", errors="replace")
        if not any(n in t for n in interp): continue
        t = CMT.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), t); n = 0
        for line in t.splitlines():
            line = line.split("//")[0]; hits = [m.group(1) for m in CALL.finditer(line) if m.group(1) != "__attribute__"]
            if line[:1].isalpha() or line[:1] == "_": hits = hits[1:]
            n += sum(1 for h in hits if h in interp)
        if n: per[f.relative_to(ROOT).as_posix()] = n
    return per
def elf_hits(elf, interp, state):
    syms, _ = elfgate.parse_elf(elf); b = elfgate.base_name
    return sorted({b(s[0]) for s in syms if s[3] == 2 and b(s[0]) in interp}), sorted({s[0] for s in syms if s[3] == 1 and s[0] in state})
```
main <elf>: files = sorted((ROOT/"3ds/src").glob("*.c")) + sorted((ROOT/"src").rglob("*.c")). Print `c0gate file <path> <n>` (top 40), `c0gate interp_syms <names>`, `c0gate state_syms <names>`, last line `c0gate calls <sum> interp <i> state <s>`. Exit 0.
`--selftest <elf>` (seeded faults): A: elf_hits with interp | {"nd_stream_run"} gives exactly 1 FUNC name more than with interp. B: src_calls of the fixture below sums to exactly 2. Print `c0gate selftest A ok` or `FAIL`, the same for B, last line `c0gate selftest ok` (exit 0) or `c0gate selftest FAIL` (exit 1).
Fixture build3ds/out/c0gate_self/fix.c, 4 lines: `void nd_nr_run(const unsigned* r) { nd_nr_exec(r, 1u); }` / `static void f(void) {` / `    nd_comb_sync(); /* nd_rnd_sync() */` / `}`.
Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`): 1. `PM_LANE=NAT0 sh 3ds/build.sh > build3ds/out/nat0_build.txt 2>&1`. 2. Edits 1-2.
3. `python 3ds/tools/c0gate.py --selftest build3ds/lane-NAT0/pm_3ds.elf > build3ds/out/nat0_self.txt 2>&1`. 4. `python 3ds/tools/c0gate.py build3ds/lane-NAT0/pm_3ds.elf > build3ds/out/nat0_c0gate.txt 2>&1`.
5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat0_host.txt 2>&1`. Gate (exact): build rc 0. Selftest last line `c0gate selftest ok`. c0gate last line: calls above 0, interp above 0, state 23. Host PASS as on main. Fix rights: the 2 tool files. A selftest FAIL or state not 23: stop, send the output path to oracle NATIVE with SendMessage.
Hand-back: commit the 2 tool files and `git add -f build3ds/out/nat0_c0gate.txt build3ds/out/nat0_self.txt`: `NAT0: c0gate tool. calls <c> interp <i> state <s> on main`. It deletes this page. One SubagentHandback: the commit hash, the c0gate last line, the 5 top `c0gate file` lines.
