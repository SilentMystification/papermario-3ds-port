# plan_pond: mac_00 pond is solid blue (oracle NATIVE21)

Fault (FACT user, O3DS builds 7812 a05f62c545 and earlier today): the pond water of mac_00 is solid blue, the water surface is gone.

## 1. How the water draws (FACT code read)
- Game: model.c custom gfx pre of MODEL_water is gfx_build_water (water_gfx.c:10-27, set at :31-32). It draws the geometry
  with combine PM_CC_PRIM_NO_ALPHA and prim (0,0,128,0) at prim depth 0x7FF0 (:13-21): the blue blocker. Then 2CYC, ZS_PIXEL,
  G_RM_PASS / XLU_SURF2 (:22-24) and mdl_draw_hidden_panel_surface (:26, model.c:3992-3998): a stack copy of the model with
  flags LOCAL_VERTEX_COPY | VALID. That copy record has no geometry (model.c:1494), no custom gfx (r[22] = r[23] = 0), no panner.
- The water model record itself: pre range R11 (model.c:1431-1434), panner tile sizes R9/R10 (:1437-1459), geometry R12.
  The panner moves aux V only (main.c:99-101: STEP 0,0,0,120). The water texture has an aux tile: the combine reads TEXEL1 (GUESS).
- Core 1: build_custom_gfx opens a sub with the unit log (model.c:3535, nd_rec.c:783 nd_ds_lg_open). The copy record is an
  NDC_MODEL inside the range. ds_apply logs each unit only while the log is open (nd_ds.c:351 lg_put).
- Core 1, water record: nd_ds_model replays the pre range from the log (nd_ds.c:486, :339-348), then the panner units (:488-489).
  nd_msnap_rec takes the snap after that (nd_rec.c:370, nd_msnap.c:19-24): words cw0/cw1, omh, oml, tile blocks, decision d_plan/d_t1.
- Core 0: rec_miss runs the pre range (nd_recs.c:68), nd_msnap_use(q) (:209), nd_msnap_dec (:212), pm_gbi_bind_tile1 (:214,
  nd_tex_set.c:27-35: no tile 1 when !plan_active or !cur_plan.uses_tex1). nd_r11_state finds the copy record (nd_model.c:300-303),
  so the draw takes its bind and combine (nd_recs.c:228-235) and the water geometry (:252-253).
- model_draw: t1_valid from the bind step (pm_gpu.c:4016). A TEXEL1 plan with no unit 1 is refused: why 6 (nd_model.c:368).
  A refused record draws nothing (pm_gpu.c:4229; rec_miss nat 0). Only the blue blocker stays.

## 2. Rows that touch a field the water needs (ranked)
| rank | row (merge) | code reason | mark |
|---|---|---|---|
| 1 | DO1 (6bdd944bec, hit copy fc1c587867) | The copy record has r[22]=r[23]=0, so the memo applies (nd_rec.c:356). A hit runs mm_fast (:338-343): mm_apply writes nd_ds words directly (:301-306), no lg_put. The water record replays the log without the copy units: its snap keeps the blocker combine PRIM_NO_ALPHA, so d_t1 = 0. Tile 1 is not bound, why 6, no surface draw. | FACT path; hit taken GUESS (strong) |
| 2 | T1FIX (984b424e1f) | nd_tex_set.c:29-30 reads the water record's own snap (before: the copy record's snap). The water then depends on the replay. Correct when the log is full. | FACT; fault only with rank 1 |
| 3 | MSNAP2 (f5b4973f04) | d_plan/d_t1 come from nd_snap_dec on nd_ds (nd_snap.c:89-105, nd_msnap.c:22). Correct when the log is full. | GUESS low |
| 4 | NC0V (77266642f7, ec755779cb) | tex_on and fog from the snap (nd_msnap.c:23, :37). Copy and water set the same texture. | GUESS low |
| 5 | MCOPY1/MCOPY2 (bd82458c72, d0483da1ba) | tile blocks and uv origin from the snap (nd_recs.c:217, nd_ds.c:485-490). Panner units stay after the replay. | GUESS low |
| 6 | DO3/DO3F (353b31c792, 9cef13d502) | a stale uv uniform freezes the scroll. That gives a still texture, not solid blue. | GUESS low |
| 7 | GD1, NC0C, NC0T1, NC0T2, KICK3, PIPE0/1 | no water field found in the read. | GUESS low |
- Why every gate passed (FACT): a drec frame forces the full memo path (nd_rec.c:359, pm_drec_now pm_gpu.c:602). The DO1 gates
  were drec only. No gate scene has a model record inside an open log.
- Push back on the brief: a drec diff cannot show the DO1 fault. Its frames take the full path. Use a shot (pic) bench in non-drec frames.
- Builds with DO1 (FACT pushed_builds.txt): 7792 (8e06a08fb3), 7812. Builds before DO1 today: 7301, 7521, 7569, 7680, 7693, 7723.
  OPEN: did the user see solid blue on 7723 or older? If yes, a second cause exists before DO1. Default: no (rank 1 only).

## 3. Mechanical bisect (row POND0, ws_POND0.md)
- New scenarios in azt.d/POND0.json: pond_mac00_det (shots 200 to 205, det) and pond_mac00_drec (drec=200:2, det),
  pos -178 0 88, entry 4 (the pond spot of pond_mac00, azt_scenarios.json:8527-8550). Benches of the same names in bench.d/POND0.json.
- base at bd0a65064e (DO1 parent). One build-at of 6bdd944bec (DO1 merge). One run at the POND0 tip. cmp each with --commit bd0a65064e.
- Expected (GUESS): DO1 merge DIFF, tip DIFF, drec tip byte-equal. A DIFF at the DO1 merge names DO1: the DO1 gates said byte-equal.
- Cost: 3 builds, 6 Azahar starts (plus 1 if cmp refuses a cold run).

## 4. Rows
- POND0 (Haiku, ws_POND0.md): the two benches and the bisect. No code. The seeded fault of the new gate is the tip (DIFF).
- POND1 (Haiku, ws_POND1.md): no memo hit while the unit log is open. nd_ds.c accessor, nd_rec.c:356 condition, host check.
  Gate: pond_mac00_det byte-equal to bd0a65064e. spot_mac01_fps60_det byte-equal to the merge-base.
- Orchestrator: add pond_mac00_det to the wave gate set after POND1 merges (shared rule text, not a coder edit).
- Counter: OPEN (this oracle did not read the C1..C12 list). Row metric: pond_mac00_det cmp DIFF to all byte-equal.
