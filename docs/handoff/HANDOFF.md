# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-17 12:30, main 82b3a5bf65 plus this file. The user stopped the previous orchestrator. A new session starts from this file.
Read this file, docs/handoff/orchestrator_rules.md, CLAUDE.md and the memory index. Then ListAgents (status `running` is the ONLY slot count).

## Goal and counters (FACT, epic_status.sh, main 82b3a5bf65)
Goal: remove the display list system. Done only when elfgate prints `func 0 data 0 gfxnames 0` and all listcen totals are 0.
Now: build 324, gfxpos 22, data 323, data_arrays 9, gfxtype 420, recdl 11, reader 792, model 15.
elfgate func 47 data 615 gfxnames 62 comes from a stale ELF. Build main to read real values.
Start of the 2026-11-16 session: build 566, gfxpos 201, data 2239, data_arrays 211, gfxtype 822.

## Merged since the last handoff (all gated byte-equal unless noted)
ENTCAPR2, INITGS, ENTSELF2 (seeded fault gives DIFF), TSTSHK2 (tst_13 proof is the host test), BASEKEEP2 (tool fix), THEATVP, GFXPOS7.
Tool fix RACE2 (4f6d4d3ff7): build_slot_reap removes a dead slot in place under a mkdir lock.
Earlier this session: process rows (BASEBIN, VOLGEN, GFXINC, GENSPEED, MAINBIN, LINKFAST, GENKEY, HOSTSEL and others), then NMDL0, NIMGM, NTABFIND, RAWIMG, FXLOC, ENTSRC and more. Git history is the log.

## Agents at handoff (ListAgents)
Running: oracle a99888970b61c266f (DESIGN1) and oracle a0c5559d594930012 (DESIGN2). The user said: do not start more work when they end.
DESIGN1 writes Sonnet pages for NMDL6/7 (entity_model.c :490 :772 :784, model.c:4340, 3 world lists) and filemenu vertex slots (filemenu_common.c:996 :1018, nd_vslot.c).
DESIGN2 writes Sonnet pages for flame.c:190 (render target buffer) and screen_overlays.c:264 (160x120 image). It also diagnoses the gbi_viewport crash under row POSTFAIL2 (3 Azahar starts).
The user approved Sonnet coders for all four designs (2026-11-17 12:20). Start them after the pages land and the user allows work.
Completed coders (a4f2ded3c4729fa08, ad433f22f062b9e41, a989e26b373144922, a3826c55761c6a920) are done. Their rows are merged.

## Open faults and decisions
- gbi_viewport crash (OPEN): one call got vp 0x00000001 in run 2 of the stored base 0610a41868 spot_mac01_drec (trace build3ds/out/postfail1_trace.txt). The full re-check of main passed (rc 0). Nothing in 3ds/src changed. The user said "figure it out". DESIGN2 owns it.
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
