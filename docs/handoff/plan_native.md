# plan_native.md: a native draw path with no running N64 state machine on core 0 (oracle NATIVE, edit in place)
Goal (user 2026-11-18): a fully native draw path without display lists, the work split by measured load. Base main 30eaaef405.
Tags: FACT (file:line), GUESS, OPEN. Core 1 = the recorder (pm_logic.c). Core 0 = the stream runner (nd_stream_run nd_rec.c:585).
## 1. Definition (FACT code read)
(a) The running state machine on core 0. It must go.
- Unit interpreter: nd_nr_exec nd_walk.c:329, nd_nr_run :347, nr_set :235, nr_geo :247, nr_ctl :283, ls_run :350, nd_nr_list :368, nd_ent_units :386,
  part_units :434, nd_ntab_run :515, gt_run :521, nd_range_run :568, nd_scomp_run nd_scomp.c:159. Other callers: nd_recs.c:29, :45, :59, nd_shape.c:871,
  nd_direct.c:222, :324, src/hud_element.c:558, :640, src/msg_draw.c:1747, src/screen_overlays.c:130, src/sprite.c:284.
- Setters that write the state unit by unit: nd_unit_same nd_set.c:123, nd_unit_apply :160 and its handlers (:33-119), nd_comb_set nd_comb.c:495, nd_tx_* nd_tex_set.c:50-227.
- Walker syncs: nd_comb_sync nd_comb.c:340 (15 call sites, grep), nd_rnd_sync nd_set.c:233, box_fold_sync nd_comb.c:379. Copies into the walker: sc_adopt nd_snap.c:154, ds_take :138, nd_direct_bind nd_direct.c:52-64.
- The state: nd_ws (OBJECT 900 bytes, .symtab of build3ds/pm_3ds.elf) and the F3 globals (design_nstate.md:10-12). 422 lines read `nd_ws.` in 15 files of 3ds/src (grep -c: nd_tex_set.c 136, nd_comb.c 99, nd_rect.c 33, nd_set.c 32, nd_direct.c 28, nd_geom.c 26, rest 68).
(b) Pure decisions. They keep no state of their own and may run on either core: nd_pso_find_i nd_psolk.c:18, nd_pso_fix, nd_comb_core and nd_comb_core1 nd_decide.c:246-248,
  nd_render_state nd_tmpl.c:292, nd_snap_bind, nd_tgt_role, nd_snap_mix, the texel test tile0_white nd_comb.c:361, nd_model_state_in nd_model.c:324.
  FACT: the combine decision reads 3 history words (leg_mode, comb_mode5, box_pat: nd_comb.c:205, :261, nd_snap.c:56). FACT: both cores run (b) today: core 1 in snap_build nd_snap.c:38-62, core 0 in nd_comb_sync and nd_rnd_sync.
End state: core 0 runs native records only (snap apply, transform, vertex and index ranges, rectangles, sprites, targets). No code reads or writes nd_ws or an F3 global. No unit interpreter exists. Core 1 keeps the N64 field model (UD1). (b) runs where UD-N1 puts it.
## 2. End proof
- Row NAT0: `python 3ds/tools/c0gate.py <elf>` prints `c0gate calls <c> interp <i> state <s>`. calls (new C13): source call sites of the `interp` names of 3ds/tools/c0gate_names.txt in 3ds/src and src.
  interp: FUNC symbols of those names in the ELF. state (new C14): OBJECT symbols nd_ws and the F3 names. End: `c0gate calls 0 interp 0 state 0`, listcen all 0, elfgate 0 0 0. (b) is not counted.
- Help counter, row NAT1: every drec run logs `c0src f<n> draws <d> mdl <m> wcomb <a> wrnd <b> wbind <c> snb <x> snn <y> snm <z>`: the drec draws whose combine, render state or bind the walker set, and the core 1 snap_build calls. A W2 deletion is byte-equal only when its family count is 0 in the gate scenes. FACT NAT1 data (18ecf83ea0): spot_mac01 draws 56 wcomb 26, entcape_kmr03 draws 98 wcomb 34 wbind 4, fxparts draws 531 wcomb 208 wrnd 219 wbind 71. FACT these are upper bounds: the bit changes before the GPU calls that flush the open batch (pm_gpu.c:4421). Core 1: snn 3633 new snaps, snm 2631 table misses in spot_mac01 (72 %). FACT NAT1B (7c774f4a74, bits after the GPU calls) c0who: spot_mac01 nd_nr_run 18, nd_ntab_rec 4. entcape_kmr03 nd_nr_run 18, op 6 = NDC_ENT 8 (nd_walk_ent nd_rec.c:576: the entity gaps nd_ent.c:412, :423-424 and the G_DL nd_walk.c:186), nd_ntab_rec 4. fxparts nd_nr_run 80, nd_ntab_rec 15, nd_parts_run_v 96, hud_rect_run 133. A count is the last walker writer, not a wrong value: a walker decision on the inputs of the last snap gives the same output.
## 3. Per-core cost and the placement of (b) (UD-N1)
| | today (FACT code) | P1: (b) on core 1 at snap time | P2: (b) on core 0 at submit time |
|---|---|---|---|
| core 1 | game step, producers, nd_ds_apply per unit (nd_rec.c:408), snap_build at every snap call: inputs, render state, 2 binds before the memo test (nd_snap.c:43-49), then find, fix, colours | same | minus find, fix, colours, render state, binds. Keeps the copy of the inputs into the snap |
| core 0 | interpreter, nd_comb_sync, nd_rnd_sync, box fold (a copy of the core 1 work), snap apply, vertex, triangle, GPU, texture | minus interpreter and syncs | minus interpreter and syncs, plus (b) once per new snap (memo as nd_comb.c:352) |
| stream | units, plus an NdSnapChk per new snap (GUESS 640 bytes, oracle_handoff.md:22) | snaps, no field units | input snaps (NdCombIn and the 3 history words), no field units |
- FACT core 1 runs under the app limit 80 % (pm_log.c:1128). FACT targets (not measured): core 1 logic 3.3 + list build 4.4 ms, core 0 emit 2.0 + frame end 1.5 ms (frame_budget.md:15-19).
- FACT perf_history.csv holds rows of 2026-11-04 only (before the core split). No `core2 lgms` line exists in build3ds/hw_live_log_131.txt. So no per-core value of a current build exists.
- Replay (nd_cmd.h:5-6): P1 replays recorded outputs (exact). P2 must save the 3 history words at the pass start and restore them for eye 2 and the key frame.
- Reading: only a console measurement decides. Row NATM (capture_NATM.md) measures first. Rule: `wwait` (core 0 waits for core 1, pm_phase.c:669) at 2 ms or more (GUESS threshold) supports P2, else P1.
## 4. Rows (all Haiku, closed pages. Gate default: drec byte-equal against `git merge-base HEAD main`, host PASS)
| wave | rows (files, FACT lines) | counter |
|---|---|---|
| W0 now | NAT0 c0gate tool (3ds/tools only). NAT1 c0src (pm_gpu.c, nd_comb.c, nd_set.c, nd_snap.c, nd_tex_set.c, nd_direct.c, nd_int.h, stub). NATM capture (no code, UD-N3) | C13, C14 base |
| W1 | Merged: NAT1B (exact c0src, c0who keys), NAT2E (particle instance snap combine, expected-diff draw 324 f500). Now, after Merge NAT1B: NAT2G every stream triangle unit takes a mesh snap (nd_rec.c:405, nd_geo.c:40, :44, nd_snap.c:75 lsover count) and NAT2A direct draws take the snap render (nd_direct.c:88, host stub and test). Dropped: NAT2B (no rectangle key in c0who). NAT2C moves to W4. NAT2D waits for the NAT2G lsover lines | c0gate 127 (NAT2G), 126 (NAT2A) |
| W2 deletes (WAIT NAT2G, NAT2A) | NAT3A nd_comb_sync in setters (nd_set.c:80, :96, :104, :158, :173, :176). NAT3B (nd_tex_set.c:151, :155, :511, :552). NAT3C box_fold_sync and nd_img_check at draws (nd_geom.c:349, :400, nd_rect.c:242-243, nd_direct.c:167-168), then NAT3D the walker arms of gv_epoch_enter (nd_geom.c:217-221). NAT3E sc_adopt, ds_take (nd_snap.c:138-166) | C13 down |
| W3 stream | NAT4A snap words geom, tss, scissor, fill, prim_z, fog factor, readers nd_geom.c:196-204, :470, :498 (nd_snap_in.c, nd_snap.c, nd_geom.c). NAT4B vertex units carry the address core 1 resolves (nd_ds.c:31, nd_walk.c:607, nd_ent.c:359). NAT4C the snap ep bit cuts the batch (nd_snap.c:119, nd_ds.c:402). then NAT4D core 1 records no field-set unit (nd_rec.c:405) | C13 down |
| W4 runners | NAT5A entity gaps (nd_ent.c:412, :423-424). NAT5B model rec_apply (nd_recs.c:29, :45, :59). NAT5C scomp (nd_scomp.c:159-167). NAT5D the src runners of section 1 and nd_direct.c:222, :324. NAT5E parts (nd_walk.c:406-479) | C13 to 0 |
| W5 | NAT6 delete nd_ws, the F3 globals and the interpreter | C14 to 0 |
| W6 (P2 only) | NAT7A-C move find, fix, colours, render state and binds from snap_build to the core 0 apply, with a memo and the history words | none |
| end | S9NAT: c0gate 0 0 0, listcen 0, elfgate 0 0 0, the 4 drec scenes equal to the last expected-diff merge | |
Files are disjoint inside each wave, except NAT3C before NAT3D (nd_geom.c). W1 to W5 do not depend on UD-N1. Each W1+ page is written from the data of the row before it.
## 5. Where byte-equal is not possible
- X1 particles (NAT2E): FACT nd_walk.c:466 sets pm_comb_vprim 1 and sc_ok 0 with no sync, so the walker draws with the vprim 0 plan. The instance snap (sn_vprim 1, nd_snap.c:69) gives the vprim 1 plan. Diff: particle draws of fxparts_drec, a TEV constant word that the stage setup does not read (FACT oracle_handoff.md:26, NREC4 round 3). GUESS same pixels. Gate: a DIFF passes only in frames that log `partsnap f<n>`. UD-N2.
- X2 lists over LS_MAX (NAT2D): no diff expected. FACT such a list has no table, so core 0 runs all its units (nd_walk.c:515) and its state is current.
- X3 entity and shape static pool: byte-equal while the pool stays. A ring switch changes the vertex bytes (pm_gpu.c:669 stat) and the draw split per epoch (nd_geom.c:284). UD-N4.
- X4 batch cut by the ep bit (NAT4C): byte-equal expected (the NST2D gate asked epbad 0, design_nstate.md:61). A DIFF stops the row.
- X5 a stale walker value that a W2 deletion shows: the NDR0G2 rule (a log line names each frame). Each case goes to the orchestrator as a DECISION with the default yes.
## 6. Risks for the O3DS (30 fps)
- R1 core 1: snap_build builds all inputs before its memo test (nd_snap.c:43-49). GUESS 500 to 1500 calls a frame (design_nstate.md R3). P1 keeps this under the 80 % limit. NAT1 counts the calls (snb).
- R2 stream: GUESS 640 bytes per new snap. 16 x 32 KB per context (FACT nd_rec.c:30-31). W3 drops the field units. memcheck decides.
- R3 core 0 falls in both placements (the interpreter and the copy of the syncs go). P2 adds (b) per new snap: GUESS the cost of nd_comb_sync today.
- R4 a ring switch (X3) adds a vertex copy per entity vertex per frame on core 0. R5 the texel test on core 1 reads texture RAM one frame before the draw (FACT NDR0D took it with snapchk bad 0, plan_finish.md:24).
## 7. User decisions (defaults)
- UD-N1 placement of (b): P1 (core 1). FACT NATM (O3DS .170, build 9cd355b1af, f1618 w300): core 0 wks 52.60 ms, core 1 lgms 40.41 ms, wwait 0.54 ms. Core 0 is the long pole, so (b) stays off core 0. Both cores are over 33.4 ms (30 fps).
- UD-N2 expected-diff gate for the particle vprim (X1). Default yes. UD-N3 one console prof capture now (capture_NATM.md, about 2 min). Default yes.
- UD-N4 entity and shape draws keep the static pool (byte-equal) or move to the ring (diff, more core 0 work). Default keep.
