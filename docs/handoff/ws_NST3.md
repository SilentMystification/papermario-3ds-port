# ws_NST3 (Haiku): debug switch dschk: core 1 nd_ds against core 0 at every command (stage S3 proof; sets the S3 counter base)
Designer: oracle PHASEE2. WAIT: branch from main after the NST1 merge (nd_ds.h, nd_ds.c, nd_ds_apply). Worktree build3ds/wt/NST3,
branch NST3. PM_ROW=NST3 PM_LANE=NST3. Edit only: nd_ds.h, nd_ds.c, nd_rec.c, nd_walk.c (3 counter lines), pm_fxtest.c (1 line),
new 3ds/src/nd_dschk.c, new 3ds/tools/nd_dschk_test.c, new 3ds/tests/host.d/nst3.sh, new azt.d/NST3.json, new bench.d/NST3.json.
## Facts (code read, main 2c1130236f; read design_nstate.md sections 3, 5, 6)
- rec_take (nd_rec.c:175) takes every record. `st_take` is 1 only for a state unit (:338). So `!st_take` in rec_take = a command.
- Core 0 raises gv_epoch for state at nd_walk.c:283 (op_typed), :297 (MOVEMEM), :353 (op_ctl). fxtest done: pm_fxtest.c:299.
## Edits
1. nd_ds.c (pure, host-built): `int nd_ds_cmp(const NdDs* a, const NdDs* b, const char** bad, int max)`: zero the draw-time fields of
   copies (design section 3 list), then compare per field with a table `{name, offsetof, sizeof}` of every NdDs field. Returns the
   count of differing fields, the first `max` names in bad.
2. nd_rec.c, core 1: `static uint8_t ds_ep, ds_sclost, ds_in;`. In nd_rec_nr after nd_ds_apply: id = w0 >> 24; set ds_ep = 1 when
   (bit 0 and id is not VTX, TRI1, TRI2, MTX, POPMTX), or (ds_sclost and id is OMH, PRIM, ENV, TEXTURE or COMB), or (id is MOVEMEM and
   (bit 0 or the unit is not a light)). At the in-line list marker (:411) set ds_ep = 1. In rec_take, inside `if (!st_take)` and only when
   `PMDL("dschk") && !ds_in`: `ds_in = 1; nd_rec_napi(nd_dschk_run, <NdDs copy + ep word>, units); ds_in = 0; ds_ep = 0;`. Then set
   ds_sclost = 1, except for a list call (nd_rec_dl): there set ds_sclost = 0 when its table has triangles (nt->ntri), else keep it.
3. nd_walk.c: `unsigned nd_st_epoch;` and `nd_st_epoch++;` next to each of the 3 gv_epoch++ lines above.
4. nd_dschk.c, core 0: `void nd_dschk_run(const uint32_t* rec)`: fill an NdDs from nd_ws and the F3 globals, nd_ds_cmp with the record
   copy; count cmds, bad (records with a difference), epbad (`(nd_st_epoch != 0) != ep`); nd_st_epoch = 0. Log the first 8 bad records:
   `dschk bad f<n> <names>`. Every 100 frames: `dschk f<n> cmds <c> bad <b> epbad <e>`. `void nd_dschk_end(void)`: `dschk end f<n> ...`.
5. pm_fxtest.c:299: after `nd_psochk_end();` add `nd_dschk_end();`.
6. nd_dschk_test.c + host.d/nst3.sh: 2 equal NdDs give 0; a copy with prim changed gives 1 and the name `prim`; a copy with only td[0]
   changed gives 0. Print `nst3 cmp ok 3/3`. The changed-prim case is the seeded fault.
7. NST3.json (azt.d and bench.d, kind azahar_stream, copies of the 4 scenes): spot_mac01_dschk (`warp=mac_01:0 pos=0:0:200 loglevel=debug
   det dschk`, until `dschk f200 `), wscmb_pause_dschk (`preset=toad padscript=wait80,START*3,wait100 loglevel=debug det dschk`, until
   `dschk f200 `), axns_sam2_dschk (the axns_sam2_drec line with `dschk` for `drec=400:3:20`, until `dschk f400 `), fxchk_a_dschk (the
   fxchk_a line with `dschk` for `psochk`, until `dschk end`, cap 45). Caps 30 unless named.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-7. Host test. Commit `NST3 1: dschk switch, 4 dschk scenes`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST3 --bench spot_mac01_drec,wscmb_pause_drec.
3. `python 3ds/tools/bench.py run <scene> --row NST3 --why "NST3 dschk base"` for the 4 scenes. `grep -h "dschk f\|dschk end\|dschk bad"`
   of the 4 run.logs > 3ds/tools/fxrest.d/dschk_base.txt. Commit it.
## Tests that decide (1 build, 6 Azahar starts)
- Host PASS with `nst3 cmp ok 3/3`. LOOP PASS: both benches byte-equal (the switch is off there).
- Each dschk run reaches its until-line and prints cmds above 0. bad and epbad are recorded, not judged: they are the S3 counter base.
## Fix rights: the listed files. An NdDs copy above 254 units, a crash or hang with dschk, no dschk line, a DIFF: stop, send the output path to the oracle. Wait.
## Stop: last commit `NST3 2: dschk base: <scene> bad <b> epbad <e> (x4)` deletes this page. One SubagentHandback with the 4 lines.
