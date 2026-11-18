# ws_NREC2 (Haiku): entity draw records carry a list snap table. Core 0 entity draws consume it (as NCONV1D2 for list calls). Counter: none of C1-C12 (stage S6 consume). Gate: drec byte-equal, snapchk bad 0, lsnap tmis 0.
Designer: oracle NREC. Branch from main 27f33ea1da or later. Worktree build3ds/wt/NREC2, branch NREC2. PM_ROW=NREC2 PM_LANE=NREC2. Runs parallel with NREC1B.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_snap.c (nd_lsc_begin, nd_lsc_unit, nd_lsc_end, lines 66-70), 3ds/include/nd_api.h (line 109), 3ds/src/nd_rec.c (nd_rec_ent_draw lines 290-303 and the weak block 675-681), 3ds/src/nd_walk.c (nd_walk_ent lines 186-199). Do not edit nd_rec.c lines 266-277 (row NREC1B).
## Facts (code read, main 27f33ea1da)
- FACT nd_rec.c:290-303: nd_rec_ent_draw records NDC_ENT with p[1] = 0, then nd_ds_list(&nd_ds, list, 0). No reader reads p[1] of NDC_ENT (stream_run nd_rec.c:704 passes p + 2).
- FACT nd_gtab.c:136-138: a list call takes its table with nd_lsc_begin, nd_ds_list, nd_lsc_end. nd_lsc_end writes the table address to nd_rec_last_pay()[4] (nd_snap.c:70).
- FACT nd_walk.c:641: gt_run brackets the list with nd_lsr_begin(table or NULL) and nd_lsr_end. ent_run (nd_ent.c:504-531) and dlc_seg (nd_shape.c:434-439) already step nd_ls_ord and consume the range snap.
- FACT nd_snap.c:69: a list with more than LS_MAX (16) ranges counts ls_over and keeps range 15 for the rest. Then later triangles take a stale snap. This row gives such a list no table.
- FACT no host unit links nd_snap.c. 5 host units link nd_rec.c: a new call in nd_rec.c needs a weak stub (pattern nd_rec.c:678).
## Edits
1. nd_snap.c:67: add `static unsigned char ls_ovl; /* NREC2: this list has more ranges than LS_MAX */` after the ls_s line. nd_lsc_begin: add `ls_ovl = 0;`. nd_lsc_unit before: `if (ls_n >= LS_MAX) ls_over++;` after: `if (ls_n >= LS_MAX) { ls_over++; ls_ovl = 1; }`.
2. nd_snap.c:70. Before: `void nd_lsc_end(void) { nd_lsc_on = 0; sn_cmds = ~0u; if (!ls_n) return; ... nd_rec_last_pay()[4] = (uint32_t)(uintptr_t)b; }` After:
```c
void nd_lsc_end_at(uint32_t* dst) { nd_lsc_on = 0; sn_cmds = ~0u; if (!ls_n || ls_ovl) return; /* NREC2: dst is the record word of the table; an over list keeps the walker binds */
    const unsigned h = ND_LS_HDR(ls_n); uint32_t* b = (uint32_t*)nd_rec_data_st(h + ls_n * (unsigned)sizeof ls_s[0]); b[0] = ls_n; b[1] = ls_ord; memcpy(b + 2, ls_f, 4u * ls_n); memcpy((char*)b + h, ls_s, ls_n * sizeof ls_s[0]); *dst = (uint32_t)(uintptr_t)b; }
void nd_lsc_end(void) { nd_lsc_end_at(nd_rec_last_pay() + 4); }
```
3. nd_api.h:109: after `void nd_lsc_end(void);` add ` void nd_lsc_end_at(uint32_t* dst); /* NREC2 */`.
4. nd_rec.c:302. Before: `nd_ds_list(&nd_ds, list, 0); /* NST2C: ... */` After: `nd_lsc_begin(); nd_ds_list(&nd_ds, list, 0); nd_lsc_end_at(p + 1); /* NST2C: ... NREC2: the list snap table in header word 1 */` (keep the old comment text).
   After line 679 (`__attribute__((weak)) void nd_msnap_tick(void) {} /* NREC1A */`) add `__attribute__((weak)) void nd_lsc_begin(void) {} /* NREC2 */` and `__attribute__((weak)) void nd_lsc_end_at(uint32_t* dst) { (void)dst; } /* NREC2 */`.
5. nd_walk.c. Above nd_walk_ent (:186) add `static __attribute__((cold, noinline)) void ent_lsr_note(void) { static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("lsnap ent f%u", pm_frame_count()); } } /* NREC2 */`.
   In nd_walk_ent, after `pm_gbi_cur_op = G_PM_ENT;` add `const uint32_t* const lsb = (const uint32_t*)(uintptr_t)((const uint32_t*)q)[-1]; nd_lsr_begin(lsb); if (lsb) ent_lsr_note(); /* NREC2: the list snap table (nd_rec_ent_draw) */`.
   After the last line `if (!drawn && words > G_PM_ENT_WORDS) nd_ops_run(q + G_PM_ENT_WORDS, 1u, 1); ...` add `nd_lsr_end(); /* NREC2 */`.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nrec2_host.txt 2>&1`: PASS. Commit `NREC2 1: entity draws take and consume a list snap table`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/nrec2_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NREC2 --bench entcape_kmr03_drec --no-host --why "NREC2 entity list snaps" > build3ds/out/nrec2_rowloop.txt 2>&1` (it builds).
3. `PM_ROW=NREC2 PM_LANE=NREC2 python 3ds/tools/bench.py run entcape_kmr03_snapchk --row NREC2 --why "NREC2 lsnap" > build3ds/out/nrec2_snapchk_run.txt 2>&1`. Then `grep -a "lsnap\|snapchk f\|snapchk bad\|ndc: stream" build3ds/azt/logs/entcape_kmr03_snapchk.txt > build3ds/out/nrec2_lsnap.txt`.
Gate (exact): host PASS. entcape_kmr03_drec byte-equal (rowloop exit 0). A `lsnap ent f` line (GUESS: the heart block, entity_model.c:469, draws each frame). Last snapchk line: bad 0. Last lsnap line: tmis 0. Report lists, cmp, p1, over (FACT NCONV1D1 base: lists 644 cmp 644 p1 644).
Stop: a DIFF, no `lsnap ent` line, bad or tmis above 0, a crash, a fatal line (`ndc: stream` too), a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `NREC2 2: gate. entcape drec equal; snapchk bad 0; lsnap tmis 0 lists <n>` deletes this page. One SubagentHandback: commit hash, the last lsnap and snapchk lines, the `lsnap ent` line.
