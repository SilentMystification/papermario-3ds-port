# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-17 about 14:00, main ab9bd8c0f5. The user stopped the previous orchestrator. A new session starts from this file.
Read this file, docs/handoff/orchestrator_rules.md, CLAUDE.md and the memory index. Then ListAgents (status `running` is the ONLY slot count).

## Goal and counters (FACT, listcen, main bb16921f75)
Goal: remove the display list system. Done only when elfgate prints `func 0 data 0 gfxnames 0` and all listcen totals are 0.
Now: build 324, gfxpos 22, data 323, data_arrays 9, gfxtype 420, recdl 11, reader 792, model 15.
All 5 ready pages merged: build 315 -> after NMDL6 and VSLOT3 -> with FLTGT and OVLIMG: build about 313, gfxpos about 11, data about 190 (GUESS, sum of page moves).
elfgate func 47 data 615 gfxnames 62 comes from a stale ELF. Build main to read real values.
Start of the 2026-11-16 session: build 566, gfxpos 201, data 2239, data_arrays 211, gfxtype 822.

## Merged since the last handoff (all gated byte-equal unless noted)
ENTCAPR2, INITGS, ENTSELF2, TSTSHK2, BASEKEEP2 (tool fix), THEATVP, GFXPOS7, RACE2 (tool fix 4f6d4d3ff7).
Earlier: process rows, NMDL0, NIMGM, NTABFIND, RAWIMG, FXLOC, ENTSRC and more. Git history is the log.

## Agents at handoff (ListAgents)
No agent runs. Oracles DESIGN1 and DESIGN2 are done (contexts above 150K: start fresh oracles with a short brief). The coders listed in older handoffs are done.
The user stopped all new work at 2026-11-17 12:30. Start coders only when the user says so.

## New working split (user 2026-11-17, committed 46ddb85612, a1ddec2f38, ab9bd8c0f5)
- Oracle designs and diagnoses. It runs no test and writes no full patch or test file (a patch only for a purely mechanical edit at many sites).
- Coder implements and runs ALL tests. At the first failure, unmatched page step or unsettled choice it writes the output to build3ds/out/ and messages the oracle (id in the launch prompt). It never diagnoses. It applies the oracle's reply and reruns. Max 3 rounds per cause.
- The oracle replies with the cause (file:line) and the exact edit text. It never applies an edit and never runs a command for the coder.
- Rules are in CLAUDE.md, .claude/agents/coder.md, .claude/agents/oracle.md and docs/handoff/orchestrator_rules.md, each rule stated once.
- OPEN: the five pages below still hold full patches from the old split. The user has not chosen: coders use those patches as the design and run the tests, or an oracle rewrites them as design-only pages. Measure rounds and tokens on the first 2 or 3 rows.
- Start a fresh oracle for each diagnosis wave. A resumed 250K-token oracle is costly (GUESS).

## Ready pages on main (all dry-run on main, host PASS)
- ws_NMDL6 (Sonnet, user approved): entity_model.c, model.c and 3 world lists. build -7, gfxpos -7, gfxtype -1. Gate direct_pra13_det and spot_mac01_drec. Run the host test with PM_CACHE=off, then rowloop --no-host.
- ws_VSLOT3 (Sonnet, user approved): filemenu D_8024B708, nd_vslot.c. build -2, gfxpos -2, data -4, data_arrays -1. Gate wscmb_fsel_drec and spot_mac01_drec. OPEN: the copy arrow has no det scene.
- ws_FLTGT (Haiku, the patch is complete): flame lists D_09000800 and D_090009E0, CIMG_UNIT in nd_gfx_conv.c. build -2, gfxpos -2, data -49, data_arrays -2, gfxtype -4. Gate fxops_drec.
- ws_OVLIMG (Haiku): screen_overlays image D_80156910 becomes 0x9600 bytes (BSS +38396). build -1, gfxpos -1, data -40. No scene runs :264. Gate host test and spot_mac01_drec.
- ws_POSTFAIL2 (Haiku, row POSTFAIL2, up to 3 Azahar starts): debug tool gbi_cold_bad_vp logs one line `POSTFAIL2 bad vp ...` to find who writes w1 = 1.
FLTGT and VSLOT3 both edit nd_gfx_conv.c: each applies after the other. Start them in 2 waves or merge one first.

## Open faults and decisions
- gbi_viewport crash (OPEN): one call got vp 0x00000001 in run 2 of the stored base 0610a41868 spot_mac01_drec (trace build3ds/out/postfail1_trace.txt). The full re-check of main passed. Code read: no producer writes 1. GUESS: a recycled stream chunk (core 1 records into chain[rec_ctx], nd_rec.c:164-172, while core 0 runs a chain, nd_rec.c:579). The user said "figure it out": row POSTFAIL2 holds the debug tool and its hit line.
- Tool faults (OPEN, no fix page): the host test cache fails inline units (/tmp/ndcombs.c missing) after an edit of model.c or nd_rec.c, workaround PM_CACHE=off or PM_HOSTSEL=all. hostsel.py:123 gave WinError 5 once (GUESS: store race).
- Parked branches ENTCAPR, ENTSELF, TSTSHK, LOOPT, SLOTFIX are superseded. The orchestrator may delete them.
- Stale pages on main (ws_FXLIT, ws_FXLT, ws_POSTASYNC and patches, ws_DLCCHK.patch): GUESS that their rows are finished. Check `git log` and delete them.
- No phase E page exists. gbi.h macro definitions leave in stage 4D. Stage 1C needs build and gfxpos at 0.
- Elfgate and counters need a fresh main build before the console push.

## Console and push
O3DS push (192.168.1.170) never succeeded. Push only when the user asks. Run a main build and binchk (docs/handoff/push_strings.txt) first.
Console tests come at the end (VINE lava piranha in kzn_19 and kmr_22, fire_breath, moon jump release y, start menu L/R and more in docs/handoff/verify_list.md).

## Pipeline facts
- Gate loop: `sh 3ds/tools/rowloop.sh --row R --bench B --why TEXT [--no-host]`. One-file edit about 54 s, warm gen 5 to 21 s.
- Merge: `sh 3ds/tools/merge_row.sh <row>` (main clean, exit 8 while build3ds/out/postmerge_FAIL.txt exists). It queues a post-merge check (14 min, cold build every 3rd). A check sends NO notification when it ends.
- A FAIL from a tool fault: read it to file and line, merge the fix with plain git, then `sh 3ds/tools/postmerge_bg.sh --queue`. A code fault is never overridden.
- Budget reset: run `powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>` FROM THE ROW WORKTREE. azt.ps1 reads the worktree copy of 3ds/logs/test_budget.csv. Reset once per designer correction.
- Coders end their turn while a job runs (status `completed`). Give each launch prompt the exact wait form: ONE Bash call holding only `until test -f <abs done file>; do sleep 10; done` (timeout 590000).

## Process rules (memory and orchestrator_rules.md)
ListAgents only for agent state. A state the user reports is fact. Short ASD-STE100 replies, sentences of 20 words or fewer, FACT/GUESS/OPEN on numbers. No agreement or apology phrases.
Never end a turn while a job or a blocked merge waits: read `postmerge_bg.sh --status` and use a wait call or wakeup (memory overnight-no-idle). On 2026-11-17 the loop idled 6.5 hours after one ended turn.
At most 3 coders and 2 oracles. Haiku coders by default. Sonnet only for user-approved designs.
