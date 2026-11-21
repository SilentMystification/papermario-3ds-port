# ws_TUB0: sub state census and the subcol_drec bench (plan_tubba.md)
Row TUB0. Model Haiku. Start main. Worktree build3ds/wt/TUB0, branch TUB0, PM_ROW=TUB0, PM_LANE=TUB0. Oracle TUBBA.
Objective: a debug census proves that a sub range draws with the state of nd_sub_open. Change no draw. C1..C12: none.
Row metric: subst lines in the shockwave slot (GUESS before: 0, the census is new; FACT after: at least 1, see the gate).
Files, in this order: 3ds/include/nd_cmd.h, 3ds/src/nd_rec.c, 3ds/src/pm_fxtest.c, 3ds/tools/azt.d/TUB0.json (new), 3ds/tools/bench.d/TUB0.json (new).
Sources are CRLF: use the Edit tool, never `sed -i`.
## 1. nd_cmd.h:49
- Before: `typedef struct { const uint32_t* a; const uint32_t* b; uint32_t* br; } NdSub;`
- After: `typedef struct { const uint32_t* a; const uint32_t* b; uint32_t* br; uint32_t e[3]; } NdSub; /* TUB0: e = prim, env, geom of nd_ds at nd_sub_close */`
## 2. nd_rec.c:801-806 (nd_sub_close, nd_sub_call). Replace the 6 lines with these lines:
```c
void nd_sub_close(NdSub* s) {
    s->b = nd_rec_mark();
    s->br[1] = (uint32_t)(uintptr_t)s->b;
    s->e[0] = nd_ds.w.prim; s->e[1] = nd_ds.w.envc; s->e[2] = nd_ds.w.geom; /* TUB0: the end state of the recorded range (before the restore) */
    nd_ds_lg_close(); nd_ds_sub_restore(); /* NST2D */
}
static __attribute__((cold, noinline)) void subst_log(const NdSub* s) { /* TUB0: at most 8 lines per frame */
    static unsigned fr = ~0u, n; const unsigned f = pm_frame_count(); if (f != fr) { fr = f; n = 0; } if (n >= 8u) return; n++;
    pm_log_evt("subst f%u prim %08x %08x env %08x %08x geom %08x %08x", f, (unsigned)s->e[0], (unsigned)nd_ds.w.prim, (unsigned)s->e[1], (unsigned)nd_ds.w.envc, (unsigned)s->e[2], (unsigned)nd_ds.w.geom); }
void nd_sub_call(const NdSub* s) { nd_rec_call(s->a, s->b); /* TUB0: an end state other than the recorded one: the range drew with the snaps of nd_sub_open */
    if (__builtin_expect(pm_drec_now(), 0) && (nd_ds.w.prim != s->e[0] || nd_ds.w.envc != s->e[1] || nd_ds.w.geom != s->e[2])) subst_log(s); }
```
- pm_drec_now has a prototype at nd_rec.c:345, pm_frame_count at :220. Keep the host seed strings of nd_rec.c (oracle_handoff.md).
## 3. pm_fxtest.c:313. After the line `pm_log("fxtest: spawn %d %s at %d %d %d", ...);` add:
  `        pm_log_evt("fxsp f%u e%d", f, i); /* TUB0: the expected-diff marker of a spawn */`
## 4. Scenario and bench (copy the shape of azt.d/NREC4.json and bench.d/NREC4.json)
- azt.d/TUB0.json: name subcol_drec, status unverified, debug `warp=mac_01:0 pos=0:0:200 fxtest=32,44,98,106,92,82,97,61,50 loglevel=debug det drec=412:9:44`,
  until `drec f764 draws [1-9]\\d* bytes \\d+`, cap 30, deadline 27, first_marker `warp: mac_01 entry 0`, expects warp, needs_picture false,
  events: warp (that marker), drec (the until). Subject: the 8 sub effects of plan_tubba.md section 2 and the control shimmer_burst, frame k 12 of each slot.
- bench.d/TUB0.json: name subcol_drec, kind azahar_pic, speed normal, default false, cap 30, the same until, tags azahar det cold drec, gates [].
## Gate (1 build, 1 Azahar row run, the base from the tool)
1. `PM_LANE=TUB0 sh 3ds/build.sh`: memcheck OK. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. c0gate: calls 0 interp 0 state 0.
2. `python 3ds/tools/bench.py base subcol_drec --commit $(git merge-base HEAD main)`: 2 equal runs.
3. `bench.py run subcol_drec --row TUB0 --why "TUB0 gate"`. `bench.py cmp subcol_drec --row TUB0`: byte-equal (the census only logs).
4. In build3ds/azt/logs/subcol_drec.txt: at least 1 `subst f<n>` line with n 400 to 443 (shockwave: pass 1 and pass 2 set other cull bits,
   shockwave.c:347-348 and :352-353, so one call always differs: the positive self-test). 0 lines with n 752 to 795 (shimmer_burst: no state
   between its calls: the negative self-test). Record every subst line and the fxsp lines in the commit message.
Fix scope: the 5 files above. Stop: a step fails, or step 4 has no line in 400 to 443, or a line in 752 to 795: SendMessage TUBBA with the log path.
Hand-back: commit hash, gate values, the subst lines. The last commit deletes this page.
