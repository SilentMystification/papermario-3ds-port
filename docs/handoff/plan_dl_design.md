# plan_dl_design: price of the display-list design paths (O3DS, Toad Town)

Plan only. No source edits, no runs. Plan source: `~/.claude/plans/no-no-hop-to-quirky-frog.md` steps 5 and 6.

## 1. Data

- Log: `build3ds/ftp_170b/hw_o3ds_log.txt`, build 1588, git bff514be5 (line 10). Switches: phase fsplit pmu, loglevel 3.
- Pricing windows (camera still, `pos` line identical: `M x 11 y 0 z 40 yaw 349 | C x 11 y 103 z 427`): f1513, f1813, f2113, f2413.
  Area 1 map 2 (load at log line 340). Each window is 300 drawn frames. Values below are per frame.
- Check window only (camera moving, `pos M x -427 y 20 z -64`): f2713. Not used for pricing.

| line | f1513 | f1813 | f2113 | f2413 | f2713 (check) |
|---|---|---|---|---|---|
| fsplit wall ms | 53.3 | 55.0 | 55.0 | 55.2 | 52.9 |
| fsplit gbi ms | 43.9 | 45.6 | 45.7 | 45.8 | 43.4 |
| opsplit 0c (G_PM_MODEL) ms | 20.5 | 20.3 | 21.1 | 20.9 | 19.7 |
| opsplit tex class ms / ops | 7.0 / 173 | 7.6 / 192 | 7.6 / 193 | 7.8 / 193 | 6.8 / 186 |
| of it 0b (G_PM_TEXI) ms | 6.3 | 6.7 | 6.7 | 6.8 | 5.8 |
| opsplit dlc class us / ops | 50 / 5 | 49 / 5 | 50 / 5 | 67 / 5 | 62 / 5 |
| other top ops ms | e4 2.8, 06 2.8, 01 2.7, 09 2.1 | e4 3.4, 01 2.8, 06 2.8, 09 2.0 | e4 3.4, 01 2.8, 06 2.7, 09 2.0 | e4 3.4, 06 3.0, 01 2.8, 09 2.0 | 06 3.0, 01 2.9, e4 2.4, 09 2.1 |
| mr0 plain models | 90 | 90 | 90 | 90 | 90 |
| mr0 us per model: ops / bind / close / outside dlc / dlc | 176 / 0 / 54 / 230 / 0 | 177 / 0 / 54 / 231 / 0 | 177 / 0 / 53 / 231 / 0 | 177 / 0 / 53 / 231 / 0 | 169 / 0 / 50 / 219 / 0 |
| ms0 opaque plain / runs / removable closes / tex changes | 71 / 13 / 15 / 47 | same | same | same | 71 / 13 / 14 / 48 |
| mdlsum models / static / walker tris | 106 / 102 / 202 | 106 / 101 / 203 | 105 / 101 / 204 | 106 / 101 / 204 | 112 / 104 / 206 |
| tx1 miss KB (window) | 44 (f1500) | 8 (f1800) | 0 (f2100) | 0 (f2400) | 16 (f2700) |

dlc2 (counters reset at each report, so the line is a 300-frame total). f1799, f2099 and f2399 are identical:
tri 255480 (852 per frame), opsrep 340800 (1136 per frame), static grp 22010 (73.4 per frame), batch 6600 (22 per frame),
ents 122, hashbad 0, exitbad 0, missing 0, fail 0. f2699 (moving): tri 815 per frame, static grp 69.1 per frame.

Derived (still windows):
- mr0 plain-model time: 90 x 230 to 231 us = 20.7 to 20.8 ms. It agrees with the 0c class (20.3 to 21.1 ms).
  Split: ops 90 x 176 to 177 = 15.8 to 15.9 ms. Close 90 x 53 to 54 = 4.8 to 4.9 ms.
- tex class without 0b: 0.7 to 1.0 ms. G_PM_TEX (0a) is not in the top 6, so walker-level 0a is below 2.0 ms.
- Walker ops (sum of class op counts) 850 to 890. opsplit ops 1313 to 1355. So about 460 nested words per frame run in mg_run.

## 2. Phase inflation (stated, not hidden)

- Phase on, this log: wall 53.3 to 55.2 ms, gbi 43.9 to 45.8 ms.
- Phase off, build 1587, a different stretch of Toad Town: wall 43.1 ms, gbi 33.8 to 34.6 ms.
- Inflation: wall +10.2 to +12.1 ms (off/on 0.78 to 0.81). gbi +9.3 to +12.0 ms (off/on 0.74 to 0.79).
- The two runs are not the same view. The ratio can be wrong by the scene difference.
- Phase cost sits on the walker op hook (os_op_hook, mops_count, mdc_op, about 1010 to 1056 tick reads per frame).
  Nested mg_run words carry only one counter store each. So nested time is likely near its phase-off value.
- Rule used below: phase-off saving = phase-on saving x 0.74 (proportional inflation) to x 1.0 (no inflation on nested time).
- Gap to the 30 fps target: 43.1 - 33.4 = 9.7 ms phase off.

## 3. Paths

### Path 1: texture bind snapshot per handle

- Model binds (G_PM_TEX inside model groups) run in mg_run through tb_run. Their time is inside 0c. `mr0 bind 0` and `mops bind 0`
  are zero by construction (no walker op), not measured zeros. This log cannot price model binds.
- Direction only: the Azahar MR0 census before MR1 (test_catalog mac01t_mr0) gave bind 10 of 37 us outside dlc (27 %).
  27 % of 20.7 ms = 5.6 ms phase on. This share is from Azahar on the pre-MR1 path. It is not a console number.
- Measured and independent of the models: 0b (G_PM_TEXI) 6.3 to 6.8 ms. Producers: src/sprite.c:236, src/entity/Shadow.c:28,
  src/hud_element.c:365 and 477, src/msg_draw.c:1758. tx1 shows 0 to 8 KB misses per window, so this is state setup, not upload.
- Estimate (assumption: a snapshot removes 50 to 80 % of a bind chain; the log does not split decode from GPU state set):
  - 0b part: 3.2 to 5.4 ms phase on, 2.3 to 5.4 ms phase off.
  - Model-bind part: 2.8 to 4.5 ms phase on, 2.1 to 4.5 ms phase off. It overlaps path 2 (a draw record contains the bind).
- Risk: medium. A stale snapshot draws the wrong texture or palette (sprite animation frames change the raster and the TLUT).
  The handle key must include raster, palette and tile state.
- Gate: scenario `mac01t_mr0` (phase on). Quantity: opsplit `top 0b` us per frame. Picture: top shot equal to mac01t outside
  the NPC strip x 92 to 138 (known run noise).
- First row: TEXI1, a per-handle snapshot in tb_inline for the sprite.c producer only (one producer per row).

### Path 2: per-model draw record

- Price base: mr0, 90 plain models, 230 to 231 us each = 20.7 to 20.8 ms phase on (ops 15.8 to 15.9, close 4.8 to 4.9).
  101 to 102 of 106 models are static (mdlsum). About 5 nested words per model, so the time per model is not word dispatch.
  It is the work inside the words: tb_run, rm_apply, matrix load, dlc_try (box test per group, static replay), batch close.
- Ceiling: 20.7 ms phase on, 15.3 to 20.8 ms phase off (the record still submits each draw, so the ceiling is not reachable).
- Estimate (assumption: a record removes 50 to 80 % of the ops part and none of the close part):
  7.9 to 12.7 ms phase on, 5.8 to 12.7 ms phase off.
- Option on top: sort a run of opaque plain models by texture (ms0: 15 removable closes per frame of 47 texture changes).
  Close cost 4.85 ms / 47 to 60 closes = 81 to 103 us each. Saving 1.2 to 1.5 ms phase on, 0.9 to 1.5 ms phase off.
  This changes the draw order. Opaque with depth test gives the same picture, but it needs a byte-equal check.
- Only this path has a range that reaches the 9.7 ms gap.
- Risk: high. Invalidation on matrix change, texture panner, tint, fog, render mode and dlc generation (pm_dlc_gen).
  A miss shows old geometry. Shared draw code (pm_gbi.c, src/model.c) needs a byte-equal check on the scenes that use it.
- Gate: scenario `mac01t_mr0` (phase on). Quantity: `mr0 outside dlc` us per model. Picture: top shot equal to mac01t.
  Console fsplit gbi at the end of the epic only.
- First row: SPLIT0C (below), then DREC1: a record for static plain models only (no panner, no tint, no fog, no billboard,
  the same set mr0 counts), keyed by model and the matrix hash that mdc_op already computes.

### Path 3: model-level bounding test

- Data source needed: the share of plain models whose dlc groups are all outside the view.
- The log cannot give it. dlc_c.skip and dlc_c.grp exist (pm_dlc.inc.c:469, 475) but dlc2 does not print them.
  No counter counts a model with all groups skipped.
- The N64 code already skips small models out of view (MODEL_FLAG_DO_BOUNDS_CULLING, src/model.c:3006). Only models without
  that flag reach the walker out of view. So the share is likely small.
- Indirect sign: static groups per frame 73.4 (still) and 69.1 (moving), tris 852 and 815. Groups drop out as the view moves.
- Estimate: not priceable. Formula: share x 90 x the cost of a model whose groups all skip (less than 230 us).
  At a share of 10 to 30 % and half cost: 1.0 to 3.1 ms phase on (assumption, no log data).
- Risk: medium. A wrong box skips a visible model. Rule risk: the user rule forbids culling that the N64 did not do (see question).
- Gate: scenario `mac01t_mr0`. Quantity: models with all groups out per frame (new count in SPLIT0C). Picture equal.
- First row: the count in SPLIT0C. No skip code before the count is in.

### Path 4: dlc replay

- opsplit dlc class: 49 to 67 us per frame, 5 ops. That is only the walker-level G_DL hits outside model groups.
- The replay inside model groups runs from mg_run (pm_gbi.c:3532) and its time sits in 0c. `mr0 dlc 0` is zero by construction.
- dlc2: 852 tris, 1136 ops replayed, 73.4 static groups, 22 batches per frame, 0 failures. All replayed groups use the static path.
- Estimate: walker-level part at most 0.07 ms. The part inside 0c is not priceable from this log. It overlaps path 2.
- Risk: low for a census, medium for a change (box test and static area are shared draw code).
- Gate: scenario `mac01t_mr0`. Quantity: dlc us per model inside mg_run (new period in SPLIT0C).
- First row: SPLIT0C. No separate code row until the split shows the dlc share of 0c.

## 4. What the log does NOT show

- Nested-word time under phase: mg_run words have no op hook. Their count is in opsplit ops (CAP1), their time is not in a class.
- The 0c class time includes all its nested words (binds, render modes, matrices, dlc replay, closes). The tex, cc, mtx and dlc
  class times exclude model content. mr0 bind and dlc are 0 for the same reason.
- No split of 0c into bind, dlc, matrix and close (only close via mr0).
- The share of models with all groups out of view (dlc skip and grp counts are not printed).
- The number of batch closes per frame (pm_fl_n is not printed). Close cost per close above uses texture changes as a proxy.
- The 0b op count (only the tex class count, which mixes 0b with other texture ops).
- The first 900 frames mix scenes (title, file select, map 5 5, then the Toad load). dlc2 f599 and f899 mix scenes.
- No phase-off reference in this run. The 43.1 ms is from build 1587 at a different stretch.
- `console x2.86` in the mr0 line is an Azahar-to-console factor. On a console log it does not apply.

## 5. Recommendation

1. SPLIT0C (logging, about 30 lines, live code agent): under pm_ph_on, mr0-style tick periods inside mg_run for G_PM_TEX, a dlc
   hit and the rest. Print dlc_c.skip, dlc_c.grp and a count of models with all groups out in dlc2. No draw change.
   Gate: `mac01t_mr0`, the new periods add up to mr0 outside dlc within 10 %. Then one console push with phase.
2. DREC1 (path 2): the first code path. It is the only path whose range (5.8 to 12.7 ms phase off) covers the 9.7 ms gap.
3. TEXI1 (path 1, 0b part): independent of path 2, measured base 6.3 to 6.8 ms. Second in line.
4. Path 3 and path 4: wait for the SPLIT0C numbers.

## 6. Question for the user

Path 3 skips a model only when the existing dlc group box test would skip all its groups. The picture stays the same.
Does this count as culling that the N64 did not do (forbidden), or is it allowed?
