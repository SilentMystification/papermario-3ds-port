# ws_ENTSRC (Haiku): the ENTRY tools reach src/ outside src/effects (no counter moves; tool row for ENTCAPE and ENTCAPR)
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTSRC, PM_ROW=ENTSRC PM_LANE=ENTSRC. Main at release: 023db9ca81.
Start: when the loop-fix work leaves 3ds/tools/gen_gfx_tab.sh (user order 2026-11-17: the loop fix first). No other file is shared.
## Objective
- FACT (code): entcap logs a list only when it has no v2 table (nd_walk.c:620 `!nt`, :509). Every itab list has one (IENTRY,
  nd_gfx_conv.c:837-839), so no itab site can be captured today. entapply.py:30 SITE takes only src/effects. gen_gfx_tab.sh:28
  scans only src/effects. gen_ntab_entries.py cannot read N(x) or ENTITY_ADDR(e, Gfx*, x) (oracle dry run: SaveBlock, Tweester: skip).
- After this row: switch `entcapt` (with `entcap`) logs the walker entry of every no-entry list call (a list with a v2 table too).
  entapply edits a site in any src/ directory. gen_gfx_tab.sh passes the files of 3ds/tools/ntab_ent.d/*.txt to gen_ntab_entries.py.
## Design (all in docs/handoff/ws_ENTSRC.patch, 8 files; edit only these)
3ds/src/nd_walk.c (entcap_rec takes the list key, switch entcapt), 3ds/tools/entapply.py (SITE), 3ds/tools/gen_ntab_entries.py
(extra files `<path>[=<NAMESPACE>]`, N(x), ENTITY_ADDR), 3ds/tools/gen_gfx_tab.sh:28, new 3ds/tools/ntab_ent.d/000_base.txt,
new 3ds/tests/host.d/entsrc.sh, new 3ds/tests/entsrc/a.txt and x/w.c (fixture).
- FACT (oracle dry run on 023db9ca81): `git apply --check` OK. gen_ntab_entries.py on src/effects: 300 lines, byte-equal to main.
  3ds/tests/gne/cases: equal to a.txt. entsrc.sh: `entsrc ok`. entapply SITE reverted in place: entsrc.sh fails (seeded fault seen).
## Edit shape (nd_walk.c:620). Before:
`    if (__builtin_expect(PMDL("entcap") && !nt, 0)) entcap_rec(t, rec[2], rec[3]);`
After: `    if (__builtin_expect(PMDL("entcap") && (!nt || PMDL("entcapt")), 0)) entcap_rec(key, rec[2], rec[3]); /* ENTSRC ... */`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTSRC.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 8 files: `ENTSRC 1: entcapt switch, entapply and gen_ntab_entries reach src/ (ntab_ent.d)`.
3. `sh 3ds/tools/rowloop.sh --row ENTSRC --bench spot_mac01_drec --why "ENTSRC gate" > build3ds/out/entsrc_loop.txt 2>&1`.
## Tests that decide
- Host: `grep -h "entsrc ok\|gne ok" build3ds/out/loop_ENTSRC_host.txt` prints both. The host summary line is PASS.
- Build rc 0. `cmp spot_mac01_drec: ... byte-equal` (base: merge-base of HEAD and main, rowloop.sh does it). `LOOP PASS`.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: the same values as main (build 386, gfxpos 89, recdl 11: FACT 9bc7a6f298).
- "no stored base" in the cmp file: run `python 3ds/tools/bench.py base spot_mac01_drec` once, then step 3 with `--no-host`.
## What you may edit to fix a fail
Only the 8 files. A host fail in entsrc.sh or gne.sh: 2 tries, then send build3ds/out/loop_ENTSRC_host.txt to the oracle.
A drec DIFF: stop. Do not change the test or the base. Send build3ds/out/loop_ENTSRC_cmp_spot_mac01_drec.txt to the oracle.
Budget: 1 build, 2 Azahar starts.
## Stop and hand-back
Last commit `ENTSRC 2: gate. spot_mac01_drec byte-equal, host PASS (entsrc ok)` deletes ws_ENTSRC.md and ws_ENTSRC.patch.
One SubagentHandback: commit hashes, the LOOP summary lines, the 3 listcen totals.
