# HANDOFF (orchestrator, overwritten at each handoff)

State at 2026-11-11 morning, main 287eb9ea9 or later. No coder runs. The oracle (Opus, agent a92b9617b58540436, past 580K tokens) is idle: a
fresh oracle starts from docs/handoff/row_ORACLE_NEXT.md. Read orchestrator_rules.md, CLAUDE.md and test_catalog.md first.

## Main now
Builds, memcheck static 20944 margin 1042, hotset M1 hot 70.1 KB (A 9.1 B 5.6 C 9.5 D 11.2 E 22.9 F 10.9 G 14.1 H 11.8 I 10.5), M2 linker 110.0 KB,
stale 0. Bases of spot_mac01_det and mdl5_iwa10_det are stored at 287eb9ea9 (two equal warm runs). The cold scenario battle_start_f182_cold can have no
base (two cold runs differ: the cold-boot fault).

## Merged on 2026-11-11 (details in git log)
Tools: TOOLB2-10 (shared run ids, slot pin, crash evidence, HOST BUSY, bench names, warm pairing, base fill order). Fixes: SAVEFIX (save block removed),
LOADMTX (guard: load_mtx skips a source outside host memory), WALLSHADE (hos_01 walls), SPLASHPREV (Sushie splash), SPARKLE (star spirit sparkles),
HUDGM and HUDRING (battle wheel circles), CRASHEXIT (a game thread fault leaves through the clean exit), ESCAPE2/3 (START+SELECT escape, Close game),
UB1, SYNC60 (60 fps pacing), KZN04 (flat per-sprite shade). Draw core: HC4f3, HC4e4, HOTORD, HC6/HC6B steps 1-4, HC8 steps 5-7, HC9B, HC9C, HCCUT1-5, HC10,
M3CEN. Closed: HC5 (T_SYNC 99.3 %, design dead), HC7b, DEADCODE (nothing dead), MOVE, AUDIT, MEM, SIZE, FX1, SBKW, TEXSTALE, STALE2, BTRANS, BG.

## Waiting on the user (console, O3DS XL .131; the XL push with log failed twice: open netloader first)
Console checks of merged changes: Shooting Star Summit hos_01 walls (WALLSHADE), battle wheel circles (HUDGM, HUDRING), save then Return to Title then load
(SAVEFIX; freshsave in debug3ds.txt now erases saves, a new-file warp state can be saved over a slot), crash test (push with crashtest=game, A on the
crash page: expect hbmenu or HOME), ESCAPE2 (3 modes, hung-game reset) and ESCAPE3 (Close game), sparkle and Sushie splash fps, kzn_04 look (flat shade),
60 fps feel, console prof push mac_01 0 0 200 (pmu I-miss and D-miss after the cuts, HC9C).
Console data: XL cold-boot log with the LOADMTX guard line (COLDBOOT), STEREOFX crash.txt (log-on push, 3D slider on, wait 12 s, START+SELECT, pull),
SHEEP prof push, PAUSEDR fps, AUDDROP line.
Decisions: KEYMISS Opus audit yes/no, battle UI wide layout (BATUI), HC8 steps 2-4 (need one census and 8 new starts).

## Open faults (facts)
Cold boot: a cold boot differs from a warm restart and two cold runs differ; load_mtx src=(Mtx*)1 reads seen only on cold boots (row_ORACLE_NEXT.md,
COLDBOOT). Battle UI: HUD wheel circles (the user's later fact); HUDGM and HUDRING fix two paths; console check decides.

## Rules changed on 2026-11-11 (in CLAUDE.md, orchestrator_rules.md, ORCH_RULES.txt, oracle.md, coder.md)
Rows may share a file in different places; oracle batches rows ahead (6 ready); orchestrator keeps coders fed; models labelled in status lines; oracle
replacement point 600K by its own count; session asks only at a real stopping point; Azahar scales by slots (3, parallel); stale build after a commit and
a stale hot_order name from the coder's own change are standard answers; base pairs two warm runs, no cold bench base.
