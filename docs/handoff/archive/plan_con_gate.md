# plan_con_gate: epic CON gate decisions from the console push

Plan only (Opus). Source: build3ds/hw_live_log_170.txt, build 1579, git f56fc77ad (HEAD: DLF1-5, CFI, CFD done; DLF6 not done). Switches: pacestat logstat dcsum fsplit pmu loglevel=debug. O3DS.
Line numbers below are the log `#N` numbers. Widescreen ON before #358 (not used for any gate). Widescreen OFF from #362.

## 1. Console data (widescreen OFF, mac_01 = area 1 map 2)
Map 2 loads at #394 and #630. Map 1 loads at #382 and #587. Windows that hold a load (max 150+ ms) are marked L and not used for a gate.

| fsplit | # | logic | dl | gbi | gwait | swap | rest | wall | gbi % | max | over50 | wm | tk |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| f3900 L | 421 | 4.3 | 3.2 | 25.9 | 1.7 | 0.8 | 2.1 | 38.3 | 68 | 162.0 | 7 | 146.8 | 1.93 |
| f4200 | 444 | 3.2 | 3.3 | 29.6 | 2.5 | 0.7 | 1.0 | 40.6 | 73 | 54.1 | 2 | 143 | 2.00 |
| f4500 | 472 | 3.2 | 3.3 | 34.0 | 2.4 | 0.8 | 1.0 | 44.9 | 76 | 54.2 | 3 | 143 | 2.00 |
| f4800 | 497 | 3.2 | 3.3 | 34.3 | 2.4 | 0.7 | 1.0 | 45.1 | 76 | 54.1 | 4 | 143 | 2.00 |
| f5100 | 526 | 3.2 | 3.3 | 34.6 | 2.3 | 0.7 | 1.0 | 45.3 | 76 | 52.1 | 7 | 143 | 2.00 |
| f5400 | 550 | 3.2 | 3.3 | 34.8 | 2.5 | 0.7 | 1.0 | 45.7 | 76 | 52.0 | 3 | 143 | 2.00 |
| f5700 | 579 | 3.2 | 3.3 | 34.7 | 2.4 | 0.7 | 1.0 | 45.6 | 76 | 52.2 | 8 | 143 | 2.00 |
| f6000 L | 615 | 4.2 | 3.3 | 29.1 | 1.2 | 0.9 | 1.8 | 40.7 | 72 | 160.2 | 8 | 155.2 | 1.96 |
| f6300 L | 655 | 4.3 | 3.3 | 29.2 | 1.4 | 0.9 | 1.5 | 40.9 | 71 | 152.0 | 4 | 160.0 | 1.96 |
| f6600 | 677 | 3.3 | 3.3 | 29.2 | 2.2 | 0.9 | 1.0 | 40.3 | 72 | 47.9 | 0 | 143 | 2.00 |
| f6900 | 711 | 3.2 | 3.4 | 35.4 | 2.6 | 0.9 | 1.0 | 46.6 | 76 | 54.1 | 12 | 143 | 2.00 |
| f7200 | 736 | 3.2 | 3.4 | 35.5 | 2.4 | 1.0 | 1.0 | 46.7 | 76 | 54.1 | 8 | 143 | 2.00 |

Steady map 2 (f4500 to f5700, f6900, f7200; 7 windows): gbi 34.0 to 35.5 ms, wall 44.9 to 46.7 ms, gbi share 75.7 to 76.4 %, gwait 2.3 to 2.6 ms, logic 3.2, dl 3.3 to 3.4, flush 0.2, audio 0.0.
pace (same span): 21.4 to 22.4 steps/s; per 5 s window 3-vblank intervals 76 to 86, 2-vblank 22 to 36, 4+ 0 to 1.

## 2. pmu lines
- 29 pmu lines (#31 to #728). rc is 00000000 on all 29 windows. Note: rc holds only the last svc of the window (the cycle-counter read, pm_phase.c:256).
- Steady map 2, widescreen OFF, per frame:
  - Window A (ev 0 = I-cache miss, ev 11 = D-cache read miss): f5100 118,970 / 48,216, cyc 11.33 M; f5700 120,597 / 49,150, cyc 11.48 M; f7500 124,701 / 49,375, cyc 11.70 M.
  - Window B (ev 16 = main TLB miss, ev 1 = stall by lack of instruction): f4800 722 / 4,622,305, cyc 11.27 M; f5400 730 / 4,665,474; f6000 725 / 4,668,685; f7200 731 / 4,755,265, cyc 11.46 M.
  - Instruction-fetch stall is 4.62 to 4.76 M of 11.27 to 11.46 M cycles: about 41 % of core 0 cycles [M].
- CFV: no console pmu line exists for the CF0 baseline (pushed_builds.txt has no CF0 push; this is the only log with pmu). "I-miss down" cannot be decided. This push becomes the baseline for the next cache rows.
- Risk [I]: cyc x 300 is about 3.4 G. A 32-bit cycle counter wraps above about 53 ms per frame. Read cyc with care on heavier scenes.

## 3. Decisions
| Row | Written gate | Data | Decision |
|---|---|---|---|
| LB1b | gwait above 1 ms (after LB1) | gwait 2.3 to 2.6 ms, tk 2.00 (bg task every frame) | GO, first code row |
| MR1 | gbi 55 % or more of the frame | 75.7 to 76.4 % | GO, second code row |
| BATTLE | wm/tk fields after a battle | no game mode 8 (BATTLE) or 9 in the log; modes seen 2, 4, 5, 7, 14, 15 | NO-GO: the gate input is missing. Needs a console push that enters and leaves a battle (forest) with fsplit |
| CORE1 | frame above 36 ms after all epics (plan_toad_33: after MR1) | wall 44.9 to 46.7 ms today; logic + dl - gwait = 3.2 + 3.3 - 2.4 = 4.1 ms (plan_toad_33 pay test needs 3 ms or more: met) | NO-GO now: its input is the frame after MR1 and LB1b. Projection 46 - 0.5..1.5 - 3..7 = 37.5 to 42.5 ms [E], so CORE1 is likely GO at the next push |
| FPS20 | user decision | wall 44.9 to 46.7 ms against the 46 ms limit (plan_toad_33 point 7); logic 3.2 ms against 4 ms | BLOCKED-ON-USER. Data: f6900 and f7200 (46.6, 46.7 ms) are above 46 ms; a stable 20 fps cap is at the edge today, and inside after LB1b |

## 4. Row LB1b (first code row)
- Files: 3ds/src/pm_nusys.c gbi_task 75-85, nuGfxTaskStart 87-107 (the non-interp branch 101-103).
- Change: when pm_interp_on <= 0 and the task has no NU_SC_SWAPBUFFER flag (the bg task, kind 0), store gfxList and gfxListSize and do not run it. In the swap task, run the stored bg list with gbi_task(..., 0) first, then the main list. If a bg task comes with no swap task after it (FS1 skip, pm_interp_key_draw), run it at the next nuGfxTaskStart or at pm_gpu_end. The interp path (88-100) keeps its key_list order.
- Mechanism: today the bg task calls pm_gpu_begin_wait (pm_nusys.c:78) before the main display list is built, so the dl build (3.3 ms) waits on the GPU tail. Running the bg gbi just before the main gbi lets the dl build overlap the tail. Expected 0.5 to 1.5 ms [E plan_toad_33].
- Size: about 20 lines.
- Gate (Azahar): mac01t_fsplit, top shots 2 of 2 byte-equal (bg sky in the shot), tk 2.00 unchanged. Azahar gwait is 0.0 (test_catalog), so gwait does not move there. The quantity is console gwait at the epic push: down from 2.3 to 2.6 ms.
- Scenario: mac01t_fsplit.
- Stop: a changed shot reverts the row.

## 5. Row MR1 (second code row)
- Files: src/model.c appendGfx_model 1384 (emit, TARGET_3DS); 3ds/src/pm_gbi.c new case beside G_PM_RM 3516 (pattern: rm_apply); include/gbi_custom.h new opcode beside G_PM_RM 204 and G_PM_TEXI 216.
- Change: plan_toad_33 MR1. One G_PM_MODEL op with a per-model record for plain static models. A differing key runs the original list.
- Note: the MR0 census gate (agent_model.md: 107 us per model outside dlc, gate 120 us) used the Azahar x 2.86 estimate. plan_dl_finish section 1 says MR1 reopens on console data. The console gbi share (76 %) is that data. The Azahar census stays a risk mark, not a gate.
- Gate: mac01t and 6 maps byte-equal; prod model ops down 600 or more (plan_toad_33). Risk: high. Scenario: mac01t.
- Stop: a changed shot or ops not down by 600: revert.

## 6. Order
1. LB1b. 2. MR1. 3. Epic push with fsplit and pmu, widescreen OFF, map 2 steady, plus one battle (BATTLE input). 4. CORE1 if wall is still above 36 ms. FPS20 waits on the user at any time.
