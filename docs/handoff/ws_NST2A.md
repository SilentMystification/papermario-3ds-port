# ws_NST2A (Haiku): static lists feed the core 1 state nd_ds (stage S3; counter: dschk bad lower than base)
Designer: oracle PHASEE2. WAIT: branch from main after the NST3 merge (dschk, fxrest.d/dschk_base.txt). Worktree build3ds/wt/NST2A,
branch NST2A. PM_ROW=NST2A PM_LANE=NST2A. Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_rec.c, 3ds/src/nd_gtab.c,
3ds/tools/nd_ds_test.c, 3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main ba48ccb6bb)
- nd_rec_dl (nd_gtab.c:108) records a table call with no unit through nd_rec_nr: core 1 nd_ds misses the list state. Table kinds: v2
  NdNDev (nd_ntab_find :67; units nt->u, count nt->nu), v1 NdGTab (nd_gtab_find :19; ops NDG_DEF d.u d.n, NDG_UNIT p, NDG_CALL p,
  nd_gtab.h:16-20, run order gt_ops nd_walk.c:571). No table: nd_rec_dl writes the G_DL word (:127), which reaches nd_rec_nr as an NDO_CALL unit.
- rec_inline (nd_rec.c:399) already feeds nd_rec_nr per unit (:412). An NDO_CALL unit with w0 bit 0 = branch (ends the list).
- A list address can be a segment address: core 0 resolves it with ptr_of (nd_walk.c:716) on nd_ws.segbase. Depth limit 12 (nd_walk.c:203).
## Edits
1. nd_ds.c: `void nd_ds_list(NdDs* s, const void* l, int depth)`. If depth >= 12 return. Resolve l with the ptr_of rule on s->w.segbase.
   v2 table: apply units nt->u[0..nu). v1 table: walk its ops in gt_ops order (NDG_DEF: its d.u units; NDG_UNIT: 1 unit; NDG_CALL:
   recurse with p's list, return after a branch). Else: the raw typed words to NDO_END (at most 200000). Per unit: NDO_CALL -> recurse
   (w1, depth + 1), and return after it when w0 bit 0 is set; any other unit -> nd_ds_apply(s, u[0], u[1]). nd_ds.c stays host-built:
   take nd_ntab_find and nd_gtab_find as declared in nd_gtab.h.
2. nd_gtab.c nd_rec_dl: add `nd_ds_list(&nd_ds, list, 0);` after the record: in the :132 branch after nd_rec_napi (before `return`),
   and after :134. Do not put it before nd_rec_napi: dschk copies nd_ds when rec_take records the command (NST3 edit 2).
3. nd_rec.c nd_rec_nr: when `(w0 >> 24) == NDO_CALL` call `nd_ds_list(&nd_ds, (const void*)(uintptr_t)w1, 1)` after nd_ds_apply.
4. nd_ds_test.c: one case: a raw typed list {PRIM, CALL(list B), END} with B = {ENV, END}: nd_ds_list sets prim and env. Fault case:
   B without its ENV must fail the expected table.
5. Correction (oracle NSTO, round 1): bad == cmds in every scene of the NST3 base (FACT: ds_fields compares tst1 as one block, one
   member differs from f1). nd_ds.c ds_fields: replace DSW(tst1) with one entry per NdTst member (nd.h:118-133):
   `#define DST(f) { "tst1." #f, offsetof(NdDs, w.tst1.f), sizeof(((NdDs*)0)->w.tst1.f) }`. nd_dschk.c: `static unsigned dk_fbad;`,
   `dk_fbad += (unsigned)nb;` after nd_ds_cmp, ` fbad %u` at the end of the 100-frame line and the end line.
## Steps (each output to build3ds/out/, read the last 20 lines). State: branch NST2A has dede6113b7 (edits 1-4, host PASS, LOOP PASS).
1. `git switch -c NST2A0 60182a2edd`. Edit 5. Host test. Commit `NST2A 0: dschk names the tst1 members; fbad`. `sh 3ds/build.sh`.
   bench.py run spot_mac01_dschk and fxchk_a_dschk. Grep their dschk lines > build3ds/out/nst2a_base2.txt.
2. `git switch NST2A`, `git cherry-pick <NST2A 0>`. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged).
   rowloop --row NST2A --bench spot_mac01_drec,wscmb_pause_drec. bench.py run the same 2 dschk scenes > build3ds/out/nst2a_dschk.txt.
## Tests that decide (2 builds, 6 Azahar starts)
- Host PASS. LOOP PASS (both benches byte-equal). In both scenes the last `dschk f`/`dschk end` fbad is lower than nst2a_base2.txt.
- Pass: write the grep of nst2a_dschk.txt (with the `dschk bad` name lines) into 3ds/tools/fxrest.d/dschk_base.txt (replace). Commit it.
## Fix rights: the listed files. A higher fbad, a crash, a DIFF: stop, send both output paths to oracle NSTO. Wait.
## Stop: last commit `NST2A 2: gate. 2 benches byte-equal; dschk fbad <2> (base <2>)` deletes this page. One SubagentHandback.
