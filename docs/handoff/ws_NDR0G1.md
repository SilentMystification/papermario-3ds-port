# ws_NDR0G1 (Haiku): the snap carries the bind block of tiles 0 and 1; snapchk compares it with the walker tiles (stage S4; counter: none of C1-C12, snapchk bad stays 0)
Designer: oracle FIN. WAIT: branch from main after the NDR0F1 merge. Worktree build3ds/wt/NDR0G1, branch NDR0G1. PM_ROW=NDR0G1 PM_LANE=NDR0G1.
Edit only: 3ds/include/nd_snap.h (struct and the nd_snap_rnd prototype area), 3ds/src/nd_snap_in.c, 3ds/src/nd_snap.c (nd_snap_mesh and sc_compare only), 3ds/tools/nd_snap_test.c.
Runs in parallel with NDR0F2 (it edits nd_snap.c below sc_compare and the nd_snapchk_rnd prototypes). WAIT also for the NDR0F1B fix in the NDR0F1 merge.
## Facts (code read, main 15887422d3 and NDR0F1 138c5a5a2a)
- ensure_tile (nd_tex_set.c:441-449) and ensure_tile_slow (:335-385) read nd_ws.td[n], tst1.tile_wrap[n], tst1.tile_sz[n]. dual_bind (nd_comb.c:408-435) also reads tst1.tile_shift[0..1].
- Core 1 keeps td[0..1] and tst1 in nd_ds.w (nd_ds.c ds_td0 :176, ds_td1 :184). dschk compares td (invalid td zeroed, nd_ds.c:524) and tst1: bad 0 (NST2J gate).
- A tile unit is not a command (nd_rec_cmds, nd_rec.c:648): the snap skip test (nd_snap.c nd_snap_mesh) must hold the bind block too.
## Edits
1. nd_snap.h before NdSnapChk: `typedef struct NdSnapBind { NdTd td; unsigned wrap[4]; int sz[4]; unsigned shift[2]; } NdSnapBind; /* NDR0G1: tile n as ensure_tile and dual_bind read it */`.
   In NdSnapChk after the NDR0F1 pad3 line: `NdSnapBind b[2]; /* NDR0G1: tiles 0 and 1 */`. The size assert stays (NdSnapBind is 84 bytes, 2 of them 168).
   After the nd_snap_rnd prototype: `void nd_snap_bind(const NdTd* td, const NdTst* t, int n, NdSnapBind* o); /* NDR0G1, nd_snap_in.c: td (zero when not valid) and the tile n words of t */`.
2. nd_snap_in.c, new (pure):
   ```
   void nd_snap_bind(const NdTd* td, const NdTst* t, int n, NdSnapBind* o) {   /* NDR0G1 */
       memset(o, 0, sizeof *o);
       if (td->valid) o->td = *td;
       memcpy(o->wrap, t->tile_wrap[n], sizeof o->wrap);
       memcpy(o->sz, t->tile_sz[n], sizeof o->sz);
       o->shift[0] = t->tile_shift[n][0]; o->shift[1] = t->tile_shift[n][1];
   }
   ```
3. nd_snap.c nd_snap_mesh, after the nd_render_state line of NDR0F1: `nd_snap_bind(&nd_ds.w.td[0], &nd_ds.w.tst1, 0, &r.b[0]); nd_snap_bind(&nd_ds.w.td[1], &nd_ds.w.tst1, 1, &r.b[1]); /* NDR0G1 */`.
   In the skip test line append `&& !memcmp(r.b, sn_last.b, sizeof r.b)` before `) return;`.
4. nd_snap.c sc_compare, before `if (sc_cur.key < 0) sc_miss++;`:
   `{ NdSnapBind w; for (int n = 0; n < 2; n++) { nd_snap_bind(&nd_ws.td[n], &nd_ws.tst1, n, &w); if (memcmp(&sc_cur.b[n], &w, sizeof w)) sc_add(n ? "b1" : "b0"); } } /* NDR0G1 */`
5. nd_snap_test.c before `(void)nf;`: one block that prints `ndr0g bind bad <b>`. Case A: td valid 1, src (const uint8_t*)0x1000, x0 3, w 16; tile 1 wrap {1,2,4,5}, sz {4,8,60,124}, shift {1,0}:
   nd_snap_bind(.., n 1, ..) gives o.td.x0 3, o.td.w 16, o.wrap[3] 5, o.sz[2] 60, o.shift[0] 1. Case B: the same td with valid 0 gives o.td.src NULL and o.wrap[3] 5.
   `--fault` expects o.td.x0 4 in case A (bad above 0). Add `bad += <b>`.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS with `ndr0g bind bad 0` and the seed seen (ndr0a.sh runs nd_snap_test --fault first). Commit `NDR0G1 1: snap bind blocks, snapchk b0 b1`.
2. `python 3ds/tools/bench.py base nimgm_strip_drec,wscmb_pause_drec` (merge-base, not charged). `sh 3ds/tools/rowloop.sh --row NDR0G1 --bench nimgm_strip_drec,wscmb_pause_drec`: LOOP PASS, byte-equal.
   Expected: byte-equal (GUESS: more snap records, each applies the same combine).
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR0G1 --why "NDR0G1 snapchk"`; `grep -a snapchk` of both logs > build3ds/out/ndr0g1_snapchk.txt.
   Pass: recs above 0, bad 0 (no b0, no b1 name). GUESS: bad 0 (dschk compares the same fields at every command).
Fix rights: the listed files; compile errors. A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`.
A DIFF, a crash, a b0 or b1 name: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR0G1 2: gate. 2 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback.
