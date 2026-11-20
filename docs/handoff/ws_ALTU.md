# ws_ALTU (Haiku: closed edits, byte-equal): core 1 drops a copy state unit that core 0 never applies (no mark, no side effect)
Designer: oracle NATIVE15 (plan_perf.md F3, rows ALTM and ALTE). Branch from main. Worktree build3ds/wt/ALTU, branch ALTU. PM_ROW=ALTU PM_LANE=ALTU.
Facts (main c23333bb73):
- PERF0 altw, entcape_kmr03_drec f201 (boot to f201): core 1 wrote model 950414 and entity 2456286 alt words, core 0 ran model 199680 and entity 483414 (21 and 20 percent).
- A copy state unit is a slot and its unit (4 words) in an nd_cpy_run record (nd_rec.c:513). Its only reader is cp_unit (nd_walk.c:265-271).
  cp_unit does nothing for an unmarked unit unless its id is OMH, OML, PRIM (cp_vside, nd_walk.c:264) or OMH, TEXTURE in mode 2 (:266-267). So any other unmarked unit is dead.
- The mark is known only after the apply, in nd_alt_mark (nd_rec.c:506). Between nd_alt_put and nd_alt_mark nothing writes the stream (nd_ds.c:430-431, :460, nd_rec.c:502).
Objective: fewer stream words and fewer core 0 copy units. Exact by construction. c0gate stays 0 / 0 / 1.
Proxy (Azahar): new altw field `d` (units dropped) > 0, and altw c1 m, c1 e fall against the base. Core 1 ms needs the console (not in this gate).
Edit only (line numbers main c23333bb73): nd_rec.c:506, :513; nd_ds.c:17; pm_gpu.c:632. Sources are CRLF.
## Edits
1. nd_rec.c:513 (the state branch of nd_alt_put) before: `{ xr_put(nd_cpy_run, ND_NOP_A | id, ..., 2u); c6_slot = wp - 2; c6_w0 = w0; c6_w1 = w1; /* NSET2 */ xr_put(nd_cpy_run, w0, w1, 1u); NDT_E(...); return; }`
   after: `{ uint32_t* const h0 = st_hdr; xr_put(nd_cpy_run, ND_NOP_A | id, (uint32_t)(uintptr_t)nd_sv_nop, 2u); c6_slot = wp - 2; c6_w0 = w0; c6_w1 = w1; /* NSET2 */`
   `xr_put(nd_cpy_run, w0, w1, 1u); c6_app = h0 != NULL && st_hdr == h0; /* ALTU: both units went into an open record */ NDT_E(nt8, NDT_ALT); return; }`
2. nd_rec.c:506: declare `static unsigned char c6_app;` with c6_slot. At the start of the `if (c6_slot) {` body of nd_alt_mark add:
   `{ const unsigned id = c6_w0 >> 24; if (!rc && c6_app && wp == c6_slot + 4 && id != ND_FS_OMH && id != ND_FS_OML && id != ND_FS_PRIM && id != ND_FS_TEXTURE) {`
   `    wp -= 4; st_hdr[0] -= 2u << 16; st_end = wp; nd_altw[7]++; c6_slot = NULL; return; } } /* ALTU: cp_unit applies no unmarked unit of this id (nd_walk.c:264-271) */`
   Keep the rest of the body. Example before: `void nd_alt_mark(unsigned rc) { if (c6_slot) { uint32_t v[2]; ...` after: `void nd_alt_mark(unsigned rc) { if (c6_slot) { { const unsigned id = ... } uint32_t v[2]; ...`.
3. pm_gpu.c:632 (altw line): append ` d %u` to the format and `, nd_altw[7]` to the arguments.
   nd_ds.c:17 comment of nd_altw: add `7 copy units dropped (ALTU)`.
## Rules the coder must not break
- A unit that opened a new record (a cut of the entity cut table, nd_rec.c:528-531) is never dropped: c6_app is 0 for it.
- Do not move nd_alt_mark calls. Do not drop OMH, OML, PRIM or TEXTURE.
## Checklist (hw_guidelines.md)
- No new loop, no division. A dropped unit skips nd_sv_enc and nd_sv_sync (nd_rec.c:506) and 16 stream bytes. The stream stays sequential (wp only moves back over its own last 4 words).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/altu_host.txt` (timeout 600000): PASS (nat5a and stunit link nd_rec.c). Commit `ALTU 1: drop dead copy units`.
2. `PM_LANE=ALTU sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
3. Base, run and cmp as ws_MDLBLK step 3, benches entcape_kmr03_drec,fxparts_drec: every shot byte-equal.
   Report the altw line of each run and of its base log. Pass also: d > 0, and tip c1 m + c1 e below the base.
Fix rights: the edited lines and compile errors. A DIFF: stop, send the output path to oracle NATIVE15.
Stop: last commit `ALTU 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp, the 4 altw lines.
