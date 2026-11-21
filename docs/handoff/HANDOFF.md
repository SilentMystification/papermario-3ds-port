# HANDOFF (overwritten at each handoff)

State: 21 Nov 2026, about 07:10. Main is `git log -1`. The reader of this file is ONE hands-on agent.
There is NO orchestrator and NO sub-agent team any more. Do not start Agent, oracle, coder or merger agents. Do the work yourself.
Start: read this file, CLAUDE.md, docs/handoff/hw_guidelines.md, docs/handoff/push_runbook.md, docs/handoff/merge_protocol.md and the memory index. Then `date`, `git log --oneline -15`, `git status`.
Ignore the role text in docs/handoff/orchestrator_rules.md (agent labels, coaches, dispatcher). The rules that stay: CLAUDE.md rules 1 to 10, the user rules in section 6.

## 1. How you work (no orchestrator)
- A goal page docs/handoff/ws_<id>.md is a closed task written for a coder. You are the coder: do its edits in a worktree `build3ds/wt/<id>`, branch `<id>`, then gate it, then merge it yourself with `sh 3ds/tools/merge_row.sh <id> [message-file]` from the main folder.
- Page edits: use the Edit tool (sources are CRLF). A mechanical fault of a page or a tool (wrong anchor, missing launch word): fix it and say so in the commit. Anything else that fails a gate: stop and report to the user with the exact output.
- Tiered gates (CLAUDE.md rule 3): every commit = build (`sh 3ds/build.sh`), host tests (`PM_HOSTSEL=all sh 3ds/tools/nd_host_test.sh`, over 180 s: run it in the background and wait for the notice), c0gate (`python 3ds/tools/c0gate.py <ELF>` must read 0 / 0 / 0). A row gate = the 1 to 2 benches the page names (`python 3ds/tools/bench.py base <bench>`, then `bench.py cmp <bench> --row <id>`, byte-exact). Budget: 8 Azahar starts per task.
- An expected-diff row passes only with the marker word: `cmp --expect <word>`; give the merge a message file with the word `expected-diff` and the marker.
- Main folder must be clean (rule 10): commit oracle or page files and run records (`git add -f 3ds/logs/perf/runs.jsonl 3ds/logs/test_budget.csv`). 3ds/logs/test_budget.csv now merges by union (.gitattributes), runs.jsonl too.
- No `git push`, no `git stash`, no hand edit of a bench base, no feature toggle, no stub.
- Hooks you will meet: no_poll_hook (no sleep, no wait loop: run a long job in the background and end the turn with `waiting for <job>`), no_guess_hook (blocks some phrases in prompts, for example "debug line", "log line", "check first"), a style hook (ASD-STE100 chat text, no apology), a stop hook (main must be clean).
- Chat style with the user: ASD-STE100 (one instruction per sentence, active voice, no semicolons, no praise, no apology, no self-blame), FACT / GUESS / OPEN on every number, questions only with AskUserQuestion (plain text is not a prompt).

## 2. Console and push (user rules, FACT)
- O3DS .170 (`o`), O3DS XL .131, N3DS .224. Push at once after the build, binchk and capture_check are done, with no question first. Ask with AskUserQuestion only when the push log shows a connect or send failure.
- Order: `sh 3ds/build.sh` (build of main) -> `python 3ds/tools/binchk.py build3ds/pm_3ds.elf --file docs/handoff/push_strings.txt` prints `binchk OK` -> capture_check (Azahar, the same launch words, in the main lane: `PM_ROW=<id> python 3ds/tools/capture_check.py --row <id> --proof "<lines>" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1 prof=1 <extra words>`; `prof=1` is needed for prof lines; a lane id without a build gives "3dsx is missing") -> push at once `powershell -NoProfile -File 3ds/go.ps1 push o -ip 192.168.1.170 -NoBuild -Seconds 150 -Profile prof -GameDebug "<words>"`.
- The push is detached (`PUSH STARTED pid N`): start the background job `powershell -NoProfile -Command "Wait-Process -Id N -ErrorAction SilentlyContinue"`, end the turn, read build3ds/out/push_o_<time>.txt and build3ds/hw_live_log_170.txt after the notice. The game serves ONE live-log client: no livelog.py while the push reads.
- To read a running game after the push ended: `py -3 -u 3ds/tools/livelog.py 192.168.1.170 40 <out-file> --idle 15` (the ring holds the last about 5000 lines). The user said "not via ftp": do not use `pull o`.
- Perf pushes always use `-Profile prof` and the mac_01 spot (`warp=mac_01:0 pos=0:0:200`). After a push: `git add -f 3ds/logs/perf/pushed_builds.txt` and commit.
- Last pushes: main 0c8a4676aa (MTX6) with transitions, logs build3ds/out/push_o_20261121-062100.txt and build3ds/out/live_mtx6b.txt. Main e04af88f51 (MTX7, MTX8) is built, binchk and capture_check OK, push failed (no answer from .170). Map transition words: `warp=mac_01:0 story=ch0_toad_town profdisk=1`, -Profile prof. A failed push (3dslink exit 255) often works on the second try after the user reopens the netloader.

## 3. State of the work (FACT from git and the ses## 3. State of the work (FACT from git and the session)
Merged on 21 Nov (all gated): DEC1, WCLIP1, PRES0, POND0, POND1, LOGDIET, L1PH0, FSUI1-3, L1C0A, L1C0B, L1Q1, SPM0, TUB0-2, DGB0, CRK1 (67f2e261d1), CRK2 (33a14931b6),
MTX1 (96d02175a5, reader stamps), MTX1K (76e3f8448c, key walks are reader entries), MTX6 (0c8a4676aa, xfer8 step census), MTX7 + MTX8 (e04af88f51).
- MTX1 old DIFF (7 of 16) came from an old base; with a new base 16/16. cmp reads the LAST stored row run: always `bench.py run` first, with PM_LANE=<id> on the build too.
- MTX1K: pm_key_show without a K copy walks the key task and reads game RAM; the key entry had rseq 0, so map/rom waits skipped it. Console norseq 50 -> 0.
- MTX7: the .nds header, .seam, .dlc file job is posted at the fade start (map_prefetch), not 4 idle steps before the load (pm_map_cover_post deleted).
- MTX8: no `pm_walk_read("mode")` at set_game_mode(CHANGE_MAP / WORLD) when pm_wide and pm_stretch stay (pm_map.c __wrap_set_game_mode).
- Tubba: subcol_drec on main has 0 subst lines to f780. Console check of the cloud fade still open.
- No branch waits for a merge. Leftover worktrees in build3ds/wt that cannot be removed (Permission denied) stay.

## 4. Open user-reported faults and their status
1. MAP TRANSITION LAG (user focus, in work). plan_maptrans.md section 0 items 5 and 6 hold the numbers. Console MTX6 data (build3ds/out/live_mtx6b.txt, before MTX7/MTX8):
   load step 140-508 ms (cause seam wait, fixed by MTX7), s1 and s43 mode wait 29-40 ms (fixed by MTX8). NOT YET MEASURED on the console: main e04af88f51
   (push failed: no answer, the user said not yet). Next: push, the user does 4 round trips mac_01 -> mac_00 -> mac_01 AFTER the live client leaves
   (push -Seconds 45, then `py -3 -u 3ds/tools/livelog.py 192.168.1.170 270 <out> --idle 45`: the ring replays slowly from #0, a short limit misses lines).
   Read xfer2 seam, xfer4, xfer5 + xfer8 (lg, gw, pres, key). OPEN after that: visible fade steps of 76-85 ms with lg 0 and gw 0 (game thread work:
   walk, GPU wait or present; the census does not split them), covered first-sight step 171 ms (158-234 model builds, 123 lists, one key walk), load
   step dma_copy+0x40 drains (23 calls, 115-150 ms). A live log client during the walks made the load step 333-501 ms in one capture (GUESS, not proven).
2. Wobble at 60 fps: PRES1 works on the console (drop 150 -> 0). Uneven K to L spacing stays until ft <= 33.4 ms. The user judges by eye.
3. dgb_01 lag (broken bridge): DGB0 census, cost linear in model count. Console capture words in plan_dgb01.md, fix needs a design and the user's yes.
4. Tubba cloud fade: TUB1/TUB2 merged, Azahar 0 subst. Needs the user's console check in a battle.
5. Star power moves lag: plan_upaway.md section 4; PARTB needs the user's yes.
6. Seams on buildings: CRK1/CRK2 census merged; CRK4-CRK6 need the user's yes (plan_seams.md).
7. Pond mac_00 (POND1), text boxes (WCLIP1, FSUI), kmr_11 outlines (DEC1): need console checks. DEC2/DEC3 have no page (plan_decal.md).
8. Status bar +4.15 ms core 0: GD9B blocked on GD9A2.

## 5. Perf data
- L1 result (FACT, console f389 set m, plan_l1.md section 5): core 0 walk -3986 I-misses per step (-7.8 %, L1C0B), core 1 -74 (L1Q1 no gain: 527 units in 55 runs,
  9.6 units per drain, per-unit code 21.2 KB > 16 KB L1). L1Q2 dropped. A core 1 gain needs fewer queue sync points or a smaller per-unit path (design, OPEN).
- Console 60 fps (O3DS .170 options fps60=1): ft about 39.6 ms, fps 48.7, lgms 33.3. Azahar runs fps60=0 unless the `fps60` word is given: a 60 fps fault needs `fps60` in Azahar.
- Fullscreen / widescreen scenarios need the debug word `nowide`, else a saved option runs Widescreen (pm_gpu.c:1941).

the user reports (game runs, console online, a fault) is fact. Never doubt it, read the code. No guess and check. No agent looks at screenshots (rule 2): proof is code read, host tests, byte compare.
- Ask every question with AskUserQuestion. A push needs no question (section 2). Take defaults for small choices. Short chat: one line per merge, fault or decision. Expand acronyms and row codes when you talk to the user.
- Sonnet rows or output-changing rows (IK1, CRK4 to CRK6, PRES2 Opus) need the user's yes. Parked, never start unasked: ASSETFREE, CIA, QTM, WALKSRC branch.
- No new model-label tricks are needed now (no agents). Memory files are in the memory folder; read MEMORY.md.

## 7. First steps for you
1. `git status`, `git lo## 7. First steps for you
1. `git status`, `git log --oneline -15`; read build3ds/out/postmerge_FAIL.txt if it exists.
2. Push main (section 2 order; push at once, ask only after a connect or send failure) and measure MTX7 + MTX8 (section 4 item 1).
3. Then the next map transition cause from the new data. Other faults of section 4 wait for the user's word.
4. Update this file at the next handoff (overwrite, keep it short).
