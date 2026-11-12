# Row S6SYNC: S6T decides where the op path decides (at its sync units, with the DIP2 skip), not once after the whole block

Model: Sonnet (pre-approved: S6 rows). Replaces row_S6KEY.md (its key words stay: continue on branch S6KEY, head 9a99f8154).
- Why not Haiku: the change is in sc_exec, the core of every compiled range (cam, sprites, entities, RM defs, effects tables).
  The coder must order the folded writes around the decision for each def kind and prove equality with s6chk. That is a
  design step in shared core 0 code.
- Why not Opus: the reference is the op path in nd_set.c and nd_comb.c, which is fixed; the gate (s6chk) exists.

## Cause (ORACLE6 code read, main 1fb48358e; S6KEY try 2: s6chk_yard bad 514 of 5493, base 211 of 1955)
- The bad rate stays about 10 % (211/1955 before, 514/5493 after): the extra key words did not change it, so the key was not
  the cause.
- Gfx_RM1_DECAL_XLU_NO_AA (src/model.c:771-776) = RenderMode, CycleType, ClearGeometryMode, SetGeometryMode.
- Op path (nd_walk_state):
  - CycleType runs set_om, which calls sync_combine because it covers the cycle bits (nd_set.c:94-95). The decision is made
    with the inherited geom, before the two geometry units.
  - set_geom (nd_set.c:32-35) does not sync.
  - sync_combine's DIP2 signature sc_sig_now (nd_comb.c:468-470) holds neither geom nor omode_l. So no later sync redoes the
    decision: the draw keeps the decision made under the old geom.
  - The work itself reads geom and omode_l (scm_sig, nd_comb.c:437-439).
- Compiled path: sc_exec (nd_scomp.c:202-221) applies the whole folded block (geom included), then keys and decides once. Its
  decision uses the new geom. The fields s6chk compares (leg_mode, plan_active, ls_use, ls_why) differ in exactly the records
  where the inherited geom differs in bits the work reads.
- Two more cases of the same kind:
  (a) A def with no sync unit (em_noncomb: ClearGeometryMode only, entity_model.c:63): the op path makes no decision, but
      sc_exec makes one.
  (b) The DIP2 skip: when sc_ok and sc_sig_now() == sc_sig at the sync unit, the op path keeps the inherited decision. sc_exec
      decides anyway.
- Op-path sync units: OMH whose mask covers the cycle bits (set_om :95), TEXTURE (nd_op_apply :172), COMB (:175). Each runs
  only when nd_op_same (nd_set.c:122-147) does not skip the unit. RARE is refused by nd_scomp_analyse. PRIM and ENV are value
  units: they run in place through nd_walk_state.

## Design (fixed; the coder chooses the inner steps)
1. nd_scomp_analyse also records the sync units of a def. Add fields at the END of NdSDef, so the `{ u, n }` initializers in
   src stay valid: `uint8_t nsync; uint8_t sync_k[4];` (the unit indices; more than 4 sync units: refuse the def, st 2). Also
   record the folded writes per segment between sync units: the coder chooses either stored per-segment masks (bounded by 5
   segments) or a run-time pass over the static units with raw writes (sc_block's per-field writes for one unit, no
   sync_combine).
2. sc_exec order:
   - pre values;
   - segment 0 writes;
   - at each sync unit, in order: the nd_op_same skip rule of that unit (skipped: no decision). Else, when
     `sc_ok && sc_sig_now() == sc_sig`, keep the decision (DIP2). Else key (nd_scomp_key + S6KEY's words, read now), find or
     make, replay, then `sc_ok = 1; sc_sig = sc_sig_now();`;
   - the next segment's writes;
   - after the last segment, the post values.
   A def with nsync 0 makes no decision and leaves sc_ok and sc_sig as they are.
   Keep `sr_ok = 0` and `gv_epoch++` as today, and keep pm_gbi_cur_op.
3. Every reader of NdSDef fields: nd_gfx_conv.c (prints NdGDef initializers: print the new fields), nd_gtab_test.c (compares
   the host analysis with the table: add the new fields), nd_scomp_test.c (initializers with all fields: add zeros; add a check
   that the RM-list form {OML, OMH cycle, GEOM, GEOM} gives nsync 1, sync_k[0] 1), nd_ent.c ent_sdef (calloc: fine),
   nd_gtab_run (S7RUN: exec_own: no change). grep `NdSDef` and `nd_scomp_analyse` before the commit; name each reader in the
   commit message.

## Start and base
Branch S6KEY in build3ds/wt/S6KEY (keeps the S6KEY key words and the test zero-init). `git merge main` first: main has S6VORD
(it edits nd_scomp_analyse and s6chk; resolve by keeping both). Budget reset once by the orchestrator.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS (SCOMP TEST PASS with the new check; gtab bad 0).
- Bases at `git merge-base HEAD main`: msgbox_talk_det, spot_mac01_det. One list `bench.py run msgbox_talk_det,spot_mac01_det,
  s6chk_yard,s6chk_mac01 --row S6KEY --why "S6SYNC: decision at the sync units"` (4 starts, all normal). cmp 1/1 and 6/6
  byte-equal. s6chk_yard `bad 0` (was 514); s6chk_mac01 `bad 0`. Hand back ranges and made of both.
- Budget 8.

## Stop
A byte difference, s6chk bad above 0 or a test FAIL: stop, commit, hand back the s6chk field lines (2 tries per named cause).
Delete this row file and row_S6KEY.md in the completing commit.
