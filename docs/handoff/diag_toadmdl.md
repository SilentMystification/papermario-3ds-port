# TOADMDL1: model census, Toad Town (mac_01)

Row TOADMDL1 (diagnostic). Counter: `mdc_*` in 3ds/src/pm_gbi.c. Mark: one call in src/model.c appendGfx_model. Phase on only. The picture is byte-equal to a HEAD build (lane mopsbase, same two shots). Azahar starts: 2 of 3 (test, HEAD baseline).
Source: build3ds/lane-mops/last_log.txt, lines #237 to #249, frame f299. Per-frame values divide by 300.

## 1. Top 12 models (per frame)
Static = frame steps with the same G_MTX hash as the last frame, out of the steps the model was drawn.

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

Per-frame counts show 0 when the value is below 1 per frame.

## 2. Totals and shares
Totals per frame: ops 1165, draws 18, tris 113, binds 78. The ops total matches the opsplit line (1165).
- Ops: top 12 hold 641 of 1165 (55.0 %). Top 5 hold 592 (50.8 %).
- Draws: top 12 hold 16 of 18 (88.9 %). Top 5 hold 16 (88.9 %).
- Tris: top 12 hold 110 of 113 (97.3 %). Top 5 hold 110 (97.3 %).
- Binds: top 12 hold 5 of 78 (6.4 %). The other 73 binds sit in models outside the top 12.
- Static: 80 of 108 models are static (at least 90 % of steps equal). In the top 12, 11 are static. Model 00b7 is not static (0 of 263 steps).
- Unattributed ops: 60 per frame. Mark table overflow: 0.

## 3. Reading
- Model 00b7 holds 373 of 1165 ops (32.0 %), 96 of 113 tris (85.0 %) and 10 of 18 draws (55.6 %).
- Model 00b7 changes its matrix every frame. It is the first target for a plan.
- The name of model 00b7 is not in the docs. Id 0x0006 is the only top model with binds.
- Pan and fog are 0 for all top 12 models.

## 4. Definitions and limits
- Draw = a run of tri ops in the op stream. A non-tri op ends the run. This is not the GPU split count, so it differs from the gpusplit draws in diag_toadnpc.md.
- Tris: G_TRI1 counts 1, G_TRI2 counts 2.
- Binds: G_PM_TEX ops (as in the mops line).
- Model attribution: the cursor moves to the model mark at the main list. Nested model draws take their own mark. The 60 unattributed ops per frame have no mark.
- Static: hash of the 16 words of each G_MTX source matrix, and of the flags. A static model has an equal hash in consecutive frames.
- Azahar numbers show direction only. The console run is not done.
