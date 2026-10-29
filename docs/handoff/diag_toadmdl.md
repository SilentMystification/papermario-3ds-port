# TOADMDL1: model census, Toad Town (mac_01)

Counter: `mdc_*` in 3ds/src/pm_gbi.c. Mark: one call in src/model.c appendGfx_model. Phase on only. Picture byte-equal to a HEAD build. Azahar starts: 2 of 3.
Source: build3ds/lane-mops/last_log.txt, lines #237 to #249, frame f299. Per-frame values divide by 300.

## 1. Top 12 models (per frame)
Static = frame steps with the same G_MTX hash as the last frame, out of the steps drawn.

| rank | id | draws | ops | tris | binds | tex | static | pan | fog |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 00b7 | 10 | 373 | 96 | 0 | 0 | 0/263 | 0 | 0 |
| 2 | 0006 | 2 | 102 | 5 | 5 | 3 | 263/264 | 0 | 0 |
| 3 | 00a1 | 4 | 88 | 8 | 0 | 0 | 263/264 | 0 | 0 |
| 4 | 0016 | 0 | 22 | 1 | 0 | 0 | 262/264 | 0 | 0 |
| 5 | 0049 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 6 | 0043 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 7 | 0048 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 8 | 0064 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 9 | 0098 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 10 | 0044 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 11 | 0066 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |
| 12 | 0027 | 0 | 7 | 0 | 0 | 0 | 263/264 | 0 | 0 |

Values below 1 per frame show 0.

## 2. Totals and shares
Totals per frame: ops 1165 (matches opsplit), draws 18, tris 113, binds 78.
- Ops: top 12 hold 641 (55.0 %). Top 5 hold 592 (50.8 %).
- Draws: top 12 hold 16 of 18 (88.9 %).
- Tris: top 12 hold 110 of 113 (97.3 %).
- Binds: top 12 hold 5 of 78 (6.4 %). Other models hold 73.
- Static: 80 of 108 models. Top 12: 11 of 12 static. Model 00b7 is not static.
- Unattributed ops: 60 per frame.

## 3. Reading
- Model 00b7 holds 32.0 % of ops, 85.0 % of tris and 55.6 % of draws. It changes its matrix every frame.
- The docs do not name model 00b7.
- Pan and fog are 0 for all top 12 models.

## 4. Definitions
- Draw = a run of tri ops. A non-tri op ends the run. This is not the GPU split count of diag_toadnpc.md.
- Tris: G_TRI1 counts 1, G_TRI2 counts 2. Binds: G_PM_TEX ops.
- The 60 unattributed ops have no model mark.
- Azahar shows direction only. Console run not done.
