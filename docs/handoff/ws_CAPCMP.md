# ws_CAPCMP (Haiku): capture compare tool. It compares 2 console prof logs only in windows with equal frame content
Designer: oracle NATIVE20. Worktree build3ds/wt/CAPCMP, branch CAPCMP. PM_ROW=CAPCMP PM_LANE=CAPCMP. Start: main. Counter: C1..C12 none (tool row).
Objective: stop false regressions. FACT: capture 3 f1289 (trect 19.3, pos -51 0 28) was compared with play 1 f689 (trect 33.0, pos 0 0 200). The status bar
drops in after 240 idle steps (src/inventory.c:737-746) and adds 26 draws per frame (pipe draws 95 to 121, live_play2_170.txt f10494, f11094).
## Directory order: 3ds/tools/capcmp.py (new), 3ds/tools/test_capcmp.py (new). Edit no other file.
## Design of capcmp.py (`python 3ds/tools/capcmp.py <log a> <log b>`; usage text and exit 2 without 2 arguments)
- Window = all `prof f<F> w<W> <tag> ...` lines of frame F. Tag = the first word. If the first word is `core2` or `stepx`, tag = the first 2 words.
- Store fields per tag, never in one dict: `ent` is in cls, c1g, c1G, c1u, c1v and ndc. Use `pb.fields` (prof_by_scene.py:20) on the whole text after
  `w<W> ` (the tag words stay in: `fps 55.2` gives fps, `core2 lgms 34.35` gives lgms).
- Signature: ndc model spr ent call trect st sw (not kb); nd dr tri; the `at` line (pb.AT, prof_by_scene.py:12): map, area, m, sp, b, e, pos0, pos.
- Skip an `a` window with chg1 or without an ndc line. A `b` window without ndc prints `b f<F> no ndc` and does not count. Match: for each `b` window, the `a` window with an equal signature and the same F; else the first equal `a` window in file order.
- Match line: `b f<F> = a f<G> | ft <a> <b> <b-a> | wk .. | fps .. | gbi .. | idle .. | pace .. | gx .. | kgx .. | gpul .. | gpuk .. | lgms .. | cyc ..`.
  Sources: fps line (fps ft wk), `ph` line (gbi), c0i (idle pace gx), c0k (kgx gpul gpuk), `core2 lgms` (lgms), pmu (pb key pmu_cyc). Values `%.2f`,
  delta `%+.2f`. A missing field prints `-`.
- No match: `b f<F> none: near a f<G> diff <k> <a>/<b> ...` (G = fewest differing signature keys, first in file order on a tie; at most 4 keys).
- Last line: `capcmp matched <n>/<m> d ft mean <x>` (m = b windows with ndc; x `%+.2f` over the matches, `-` when n is 0).
Parser shape (pmu_report.py:19 imports from prof_by_scene the same way; add sys.path.insert(0, the tool folder) first):
```python
import prof_by_scene as pb
TWO = ("core2", "stepx")
def windows(path):
    win = {}
    for ln in open(path, errors="replace"):
        m = pb.PROF.search(ln); t = m.group(3).split() if m else []
        if not t: continue
        n = 2 if t[0] in TWO and len(t) > 1 else 1
        win.setdefault(int(m.group(1)), {})[" ".join(t[:n])] = (pb.fields(m.group(3)), ln)
    return win
```
## Tests (write output to build3ds/out/ of the worktree, read the last 20 lines)
1. `python 3ds/tools/test_capcmp.py` prints `captool capcmp ok`, exit 0. Embedded log strings (as test_capture_check.py). Cases: T1 equal signature, ft 42.18
   and 38.98: the line holds `b f689 = a f689` and `ft 42.18 38.98 -3.20`, last line `capcmp matched 1/1 d ft mean -3.20`. T2 seeded fault: b trect 19.3:
   `b f689 none` and `trect 33.0/19.3`, `matched 0/1`. T3 only kb differs: match. T4 a window chg1: no match. T5 b without ndc: `no ndc`, m 0.
   T6 cls `ent 1.72` and ndc `ent 11.0` in both logs, cls ent differs: match.
2. Real logs (main checkout, from the worktree root): `python 3ds/tools/capcmp.py ../../out/live_cap1c_170.txt ../../out/live_play1_170.txt` gives
   `b f689 = a f689 | ft 42.18 38.98 -3.20`, `b f389 none` (st 838.3/838.4) and `capcmp matched 19/20`. FACT from the ndc lines.
3. `python 3ds/tools/capcmp.py ../../out/push_o_20261120-214452.txt ../../out/live_play1_170.txt` gives `capcmp matched 0/20`.
4. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS once at the gate commit. Build and c0gate 0 / 0 / 0 once (no C change: same values).
Fix rights: the tool and its test only. A real-log line that differs from test 2 or 3: send the output path to oracle NATIVE20. Do not change the expected text.
Stop: `CAPCMP 1: gate` commit (both files, this page deleted). Hand-back: one SubagentHandback: commit hash, the 3 last lines of tests 1 to 3, host result.
