# ws_DEL5B2 (Haiku): no device code reads a v1 table (NdGTab). Particles carry the v2 entry; the v1 arms of the list runner, core 1, the entity compile and nd_rec_dl go; nd_rec_dl is nd_rec_ntab. Counters: C11 elf func 45 -> 42 (FACT elfgate on basebin 7dd3cc76d7956f86: gt_ops, nd_gtab_run, nd_rec_dl are in the ELF), C8 reader -7 (nd_walk.c gt_call 3, gt_ops 4), C7 gfxtype -2 (FACT listcen line scan 3c2469a282). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. Start from main (DEL5B1, DEL1B, DELLC, DELT4, DELC2, DEL1C, DEL5A merged; DEL3A has no page yet: this row goes first in nd_rec.c). The line numbers of this page are of 3c2469a282: find the quoted text and the named function, the numbers moved. NCONV2D is merged: keep nd_ntab_run (nd_walk.c:642) as it is. Worktree build3ds/wt/DEL5B2, branch DEL5B2. PM_ROW=DEL5B2 PM_LANE=DEL5B2.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_rec.c (parts_tab, nd_parts_add), 3ds/src/nd_walk.c, 3ds/src/nd_ds.c, 3ds/src/nd_ent.c, 3ds/include/nd_int.h, 3ds/src/nd_gtab.c, 3ds/include/nd_gtab.h, 3ds/tools/nd_ds_test.c.
## Facts (code read, main 3c2469a282; find the quoted text, line numbers move)
- FACT the device v1 index holds only the 182 moved parts lists, keyed &ndd_<name> (DEL5B1 facts). Each has a v2 entry, found first by nd_rec_dl (nd_gtab.c:120), ds_list_at (nd_ds.c:431) and gt_run0. So nd_gtab_run, gt_ops, gt_call and ds_gtab never run on the device.
- FACT ent_compile (nd_ent.c:319) takes `gt = k->v2 ? NULL : nd_gtab_find(l0)`. A device v1 key is a flat parts list (only VTX and TRI units, nd_gtab.h:44): ent_flat_w stays 0, so k->v2 = 1 and gt is NULL. Then every gap is bad (:324), k->pc is NULL, pcn = 0: ent_run never calls nd_ent_pieces (:517, :524), every gap runs ent_gap.
- FACT after DEL5B1 the parts fl has NDN_FL_PARTS exactly for the lists of the 182 v1 tables (nd_gfx_conv.c g_parts, every list moved).
## Edits
1. nd_rec.c parts_tab (:366-374): `const NdGTab*` -> `const NdNDev*` (return type, memo_t, t); `nd_gtab_find(l)` -> `nd_ntab_find(l)`; `!(t->flags & NDG_F_PARTS)` -> `!(t->fl & NDN_FL_PARTS)`. nd_parts_add (:384): `const NdGTab* t` -> `const NdNDev* t`. The comments :359-360 and :391 name the v2 entry (DEL5B2).
2. nd_walk.c parts runners (the 2 DEL5B1 lines) -> `const NdNDev* t = (const NdNDev*)(uintptr_t)p[2u * pre + 1u]; /* DEL5B2: nd_parts_add put the v2 entry in the call unit */`. Delete gt_ops (decl :574 and :589-616) and gt_call (:575-588). gt_body (:644-647) -> `static void gt_body(const NdNDev* nt) { nd_gtr_ntab++; nd_ntab_run(nt); } /* DEL5B2: every list runs its v2 entry */`. gt_run0 and gt_run: drop the parameter `const NdGTab* t`, pass `gt_body(nt)`. Delete nd_gtab_run (:693) and nd_gtab_run_e (:698). nd_ntab_rec and nd_ndev_rec: `gt_run(x, NULL, y, rec)` -> `gt_run(x, y, rec)`. Delete nd_ent_pieces with its comment (:617-640).
3. nd_ds.c: delete the ds_gtab declaration (:412), the 2 lines `const NdGTab* g = nd_gtab_find(l);` and `if (g) { ds_gtab(s, g, depth); return; }` (:433-434), ds_gtab (:437-452). Comment :405-406: delete `a v1 table its ops in gt_ops order (NDG_DEF d.u, NDG_UNIT, NDG_CALL);`. The DEL5B1 line in nd_ds_parts -> `const NdNDev* e = (const NdNDev*)(uintptr_t)p[2u * j + 1u];` (the loop after it stays).
4. nd_ent.c: delete the S7ENT block (:217-267, the comment, EntTP to the end of ent_tdwalk; keep :268). ent_sdef_free: delete ` free(k->pc); k->pc = NULL; k->tpc0 = k->tpcn = 0;`. Replace :319-345 (from `const NdGTab* gt = ...` to the end of the gap loop) with:
```c
    nd_ent_notab++; /* DEL5B2: no v1 table on the device (gt was NULL, every gap ran ent_gap) */
    for (unsigned g = 0; g <= ns; g++) {
        const u32 ga = g ? ent_sbuf[g - 1].op1 : 0u, gb = g < ns ? ent_sbuf[g].op0 : nops;
        if (gb > ga) nd_ent_gapn++;
        if (g < ns) { k->seg[g].a0 = ga; k->seg[g].op0 = gb; k->seg[g].op1 = ent_sbuf[g].op1; k->seg[g].tri0 = ent_sbuf[g].tri0; k->seg[g].ntri = ent_sbuf[g].ntri; k->seg[g].sd = ent_sdef(l, ga, gb); }
        else k->tail = ent_sdef(l, ga, gb);
    }
```
   ent_run :517 -> `if (e->op0 > e->a0) ent_gap(e->sd, fw + e->a0, e->op0 - e->a0);`. :524: `{ if (k->tpcn) nd_ent_sc += ...; else ent_gap(k->tail, ...); }` -> `ent_gap(k->tail, k->fw + e->op1, k->nops - e->op1);`.
5. nd_int.h: EntSeg (:318) delete `u16 pc0, pcn;` and the S7ENT part of its comment. Delete :319-321 (ENP enum, EntPiece, nd_ent_pieces). EntKey: delete :338-339 (pc, tpc0, tpcn).
6. nd_gtab.c nd_rec_dl: rename to nd_rec_ntab (definition :109 and the 2 calls :140-141). Delete `else { const NdGTab* g = nd_gtab_find(list); ... }` (:122) and `const int v2 = (fl & 2u) != 0;`. The record line -> `nd_rec_napi(dl_ent ? nd_ntab_rec_e : nd_ntab_rec, r6, ls ? 3u : ent ? 2u : 1u);`. memo_t comment -> `/* the NdNDev* of the list */`. nd_gtab.h: `void nd_rec_dl(const void* list);` -> `void nd_rec_ntab(const void* list); /* DEL5B2: was nd_rec_dl */`; delete the nd_gtab_run and nd_gtab_run_e prototypes.
7. nd_ds_test.c: `nu[5]` and `nu[11]` take `(uint32_t)(uintptr_t)&nrec4_dev`; delete the nrec4_tab line.
8. `git grep -nE "nd_gtab_find|NdGTab|NdGOp|NdGDef|gt_ops|nd_gtab_run|nd_rec_dl\b|nd_ent_pieces|EntPiece" -- 3ds/src 3ds/include > build3ds/out/del5b2_grep.txt`: only nd_gtab.c (nd_gtab_find, gt_sort: DEL5B3), nd_gtab.h (the v1 types, nd_gtab_idx), nd_lkey.h (nd_lkey_tab) and comments.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-8. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del5b2_host.txt 2>&1`: PASS, `nrec4 parts tris 4 units 4 bad 0`. Commit `DEL5B2 1: no device reader of the v1 tables`.
2. `python 3ds/tools/bench.py base fxparts_drec --commit $(git merge-base HEAD main) > build3ds/out/del5b2_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DEL5B2 --bench fxparts_drec --no-host --why "DEL5B2 v1 readers" > build3ds/out/del5b2_loop1.txt 2>&1`.
3. The same 2 commands for entcape_kmr03_drec (del5b2_base2.txt, del5b2_loop2.txt). Then `python 3ds/tools/elfgate.py build3ds/lane-DEL5B2/pm_3ds.elf > build3ds/out/del5b2_elf.txt` and `python 3ds/tools/listcen.py > build3ds/out/del5b2_listcen.txt`.
## Gate (exact)
Host PASS with the nrec4 line. Both rowloops exit 0. The grep of edit 8 as stated. del5b2_elf.txt names none of gt_ops, nd_gtab_run, nd_rec_dl. listcen: report the reader and gfxtype totals (expected 7 and 2 below the start commit; data, not judged).
## Fix rights and stop
Fix rights: the listed files, compile errors and unused names that this page deletes. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time. One bench per rowloop call.
## Hand-back
Last commit `DEL5B2 2: gate. fxparts and entcape drec equal; elf func -3` deletes this page. One SubagentHandback: commit hash, the elfgate line, the listcen totals.
