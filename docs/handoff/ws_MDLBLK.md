# ws_MDLBLK (Haiku: deletion, byte-equal): delete the full-state command block of cached model draws; every draw emits deltas against the shadow
Designer: oracle NATIVE15 (plan_perf.md F5, F6). Branch from main. Worktree build3ds/wt/MDLBLK, branch MDLBLK. PM_ROW=MDLBLK PM_LANE=MDLBLK.
Facts (main c23333bb73, PERF0 altw line of entcape_kmr03_drec at f201): 8853 emitted draws use 658262 words (74 per draw); 4060 block draws use
740594 words (182 per draw). blk_put copies a full-state block and resets the shadow (pm_gpu.c:1314-1327), so the next draw sends its full state again.
Objective: fewer PICA command words, one draw path, 256 KB less heap (BLK_WORDS 65536 x 4, pm_gpu.c:1285). c0gate stays 0 / 0 / 1.
Proxy (Azahar): altw `bd` becomes 0; `gw` of the tip is below `gw + bw` of the base. Console gain is not needed for the gate.
Edit only (line numbers main c23333bb73): pm_gpu.c:1278-1327, :1355-1357, :4232, :4234; nd_emit.c:38-43, :53, :82, :84, :94, :122, :138-179; nd.h:324-326, :435;
nd_model.c:677; the blkchk_mac01 objects in 3ds/tools/azt_scenarios.json (name line 14559) and 3ds/tools/bench_defs.json (name line 8253). Sources are CRLF.
## Edits
1. pm_gpu.c: delete the comment block and code from `/* HC6 step 4 (plan_hotcode 5.4): the pre-built command block` (:1280) to the `}` that ends blk_put (:1327):
   BLK_WORDS, blk_ar, blk_n, blk_gen, ndn_bq, bc_*, blk_build, blk_check, blk_put. Keep sub_pm5 above and c0_count below.
2. pm_gpu.c:1356-1357 before: `const int bq_ = ndn_bq && ... ? blk_put(cb + coff, ndn_bq, d) : nd_emit_draw(cb + coff, &ndn_sh, d);` and `... sc_gd[bq_]++; sc_gw[bq_] += nw_; } /* HC6 step 4 ... */`
   after: `const unsigned nw_ = nd_emit_draw(cb + coff, &ndn_sh, d);` and `GPUCMD_SetBufferOffset(coff + nw_); sc_gd[0]++; sc_gw[0] += nw_; } /* MDLBLK: one emit path */`.
3. pm_gpu.c:4232 `ndn_bq = q; /* HC6 step 4 ... */` and :4234 `ndn_bq = NULL;`: delete both lines.
4. nd_emit.c: emit_core loses the parameter `NdBlkOff* off`. Delete the OFF macro (:40), its comment (:38-39), `if (off) ...` (:43), every `OFF(...)` call (:53, :82, :84, :94, :122)
   and `#undef OFF` (:138). nd_emit_draw (:139) becomes `return emit_core(out, sh, d);`. Delete nd_emit_full (:140-145), dec, nd_emit_dk/da/db and nd_emit_cmp (:146-179).
   Example (:82): before `if (u == 0) OFF(u0, o - out);` after: the line is gone. (:53) before `{ val = d->tev_col[cs]; OFF(col[cs], o - out); }` after `{ val = d->tev_col[cs]; }`.
5. nd.h: delete :324 (NdBlkOff), :325 (nd_emit_full), :326 (nd_emit_cmp). :435 NdNatC: delete `uint32_t boff; int bt; uint16_t blen, bgen; NdBlkOff bo;` and fix the
   comment of the struct (no block). nd_model.c:677: delete `q->bgen = 0; /* HC6 step 4: no command block yet */`.
6. JSON: delete the blkchk_mac01 object from azt_scenarios.json and from bench_defs.json by its line range (the `{` line to its `},` line), then
   `python -c "import json; json.load(open('3ds/tools/azt_scenarios.json')); json.load(open('3ds/tools/bench_defs.json'))"`. Also drop the name from any list that holds it
   (`grep -n blkchk_mac01 3ds/tools/*.json` must print nothing).
## Checklist (hw_guidelines.md, PICA)
- No new per-draw work: the emit path already runs for every non-block draw. No new buffer, no division, no double.
- PICA: the shadow (nd_emit.c:55, :80, :93, :104) now spans model draws, so a register is sent only when its value changes.
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/mdlblk_host.txt` (timeout 600000): PASS (nd_emit_test uses nd_emit_draw only). Commit `MDLBLK 1: one emit path`.
2. `PM_LANE=MDLBLK sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
3. `PM_ROW=MDLBLK PM_LANE=MDLBLK python 3ds/tools/bench.py base <bench> --commit $(git merge-base HEAD main)` for spot_mac01_drec and entcape_kmr03_drec,
   `PM_ROW=MDLBLK python 3ds/tools/bench.py run spot_mac01_drec,entcape_kmr03_drec --row MDLBLK --why "MDLBLK: model draws emit deltas"`,
   `python 3ds/tools/bench.py cmp <bench> --row MDLBLK --commit $(git merge-base HEAD main)`: every shot byte-equal.
4. Report the altw line of each run and of its base log (basecache): gd, gw, bd, bw. Pass: bd 0 and tip gw < base gw + bw.
Fix rights: the edited lines, compile errors, a host test that names a deleted symbol (delete its use). A DIFF or tip gw not below base gw + bw:
stop, send the output paths to oracle NATIVE15.
Stop: last commit `MDLBLK 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp, the 4 altw lines.
