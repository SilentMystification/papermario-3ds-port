# ws_NAT5F (Sonnet: 6 files, 2 cores, an exact copy rule, a user-gated edit): a static list call writes its own units into the stream as a raw alt range. Core 0 runs that range with today's executor (ls_run with a table, else nd_nr_exec), not the static table. Also rec_lights, and the glyph fallback after the user's yes. Counter: c0gate calls -2 (rec_lights); -3 with the glyph edit. Gate: drec byte-equal; the glyph edit adds one exception.
Designer: oracle NATIVE3 (agent a86ed4bedc3a8575e). WAIT: none. Start commit main. Worktree build3ds/wt/NAT5F, branch NAT5F. PM_ROW=NAT5F PM_LANE=NAT5F. Edit only: 3ds/src/nd_ds.c, 3ds/src/nd_rec.c, 3ds/src/nd_gtab.c, 3ds/src/nd_walk.c, 3ds/include/nd_api.h, 3ds/src/nd_mtx_set.c, src/msg_draw.c (edit 6 only).
## Facts (main cbacc2f7c8)
- FACT nd_rec_ntab (nd_gtab.c:139-143) records the list call (r6, 1 to 3 units), then nd_ds_list applies the list units to nd_ds and builds the snap table. ds_pairs (nd_ds.c:413-425) walks nt->u, which is the same array that core 0 runs (gt_body nd_walk.c:475: nd_ntab_run: ls_run with a table, else nd_nr_exec). A CALL unit recurses at depth + 1.
- FACT gt_run0 runs gt_body at ex_depth d + 1 for a drawing list (:514) and at ex_depth d for a state-only list (:487). nd_nr_run (:367) is the stream NRUN runner. NAT5R gives nd_alt_run (nd_rec.c:621). The NAT5R alt range is byte-equal in 4 drec benches (merge 23359e6e87).
- FACT rec_lights (nd_mtx_set.c:204-205): nd_unit_same is 0 for MW NUMLIGHT (nd_set.c:143), and nd_unit_apply runs nd_light_count(w1) (nd_set.c:110).
## Edits (new lines end with `/* NAT5F */`)
1. nd_ds.c: next to nd_lsc_on (:16) add `unsigned char nd_alt_raw_on; __attribute__((weak)) void nd_alt_put(uint32_t w0, uint32_t w1) { (void)w0; (void)w1; } /* NAT5F */`. In ds_pairs, after the `raw && END` line: `if (nd_alt_raw_on && depth == 0u) nd_alt_put(w0, w1); /* NAT5F: the top units, raw, as core 0 runs them */`.
2. nd_rec.c, after nd_rec_alt:
```c
/* NAT5F, core 1: a raw alt range. The units are copied as they are (no mark, no shadow drop, no snap); core 0 runs them with its list executor.
 * A texture rectangle and its 2 RDPHALF units stay in one NRUN run (nr_ctl reads at[3], at[5]). */
void nd_alt_put(uint32_t w0, uint32_t w1) {
    if ((w0 >> 24) == ND_FS_RECT && (w1 & ND_FS_RECT_TEX) && st_hdr && (((st_hdr[0] >> 16) & 0xffu) > 252u || wp + 6u > wend)) st_hdr = NULL;
    st_put(w0, w1); }
uint32_t* nd_alt_open(void) { rec_open(); uint32_t* const br = rec_take(2); br[0] = NDC_HDR(NDC_JUMP, 0, 0); br[1] = 0u; st_hdr = NULL; sh_all(); return br; }
void nd_alt_close(uint32_t* br, uint32_t* ab) { rec_open(); st_hdr = NULL; sh_all(); br[1] = (uint32_t)(uintptr_t)wp; ab[0] = (uint32_t)(uintptr_t)(br + 2); ab[1] = (uint32_t)(uintptr_t)wp; }
```
3. nd_api.h, after nd_alt_run (:91): `uint32_t* nd_alt_open(void); void nd_alt_close(uint32_t* br, uint32_t* ab); extern unsigned char nd_alt_raw_on; /* NAT5F, nd_rec.c and nd_ds.c, core 1 */`.
4. nd_gtab.c :139-143. r6 becomes `uint32_t r8[8]` with the same 6 words and `0u, 0u` (rec[6], rec[7]: the alt bounds). nd_rec_napi gets r8 and `4u` units. Then:
```c
    uint32_t* const pay = nd_rec_last_pay(); uint32_t* const br = nd_alt_open(); nd_alt_raw_on = 1; /* NAT5F: the units as a raw alt range */
    if (ls) nd_lsc_begin();
    nd_ds_list(&nd_ds, list, 0); /* NST2A: the list's state units follow its record */
    nd_alt_raw_on = 0; nd_alt_close(br, pay + 6); /* NAT5F */
    if (ls) nd_lsc_end_at(pay + 4); /* NAT5F: the table after the range */
```
5. nd_walk.c. Above nd_nr_run add `static unsigned char alt_raw; static void ls_run(const uint32_t* q, unsigned n); /* NAT5F */`. nd_nr_run becomes `void nd_nr_run(const uint32_t* rec) { const unsigned n = (rec[-2] >> 16) & 0xffu; if (alt_raw && nd_ls_on) ls_run(rec, n); else { ep_stream = !alt_raw; nd_nr_exec(rec, n); ep_stream = 0; } } /* NAT4A; NAT5F: a raw alt run takes the list executor */`. gt_body becomes `static void gt_body(const uint32_t* rec) { nd_gtr_ntab++; if (nd_ls_on) nd_lsr_geo(); alt_raw++; nd_alt_run(rec + 6); alt_raw--; } /* NAT5F: as nd_ntab_run, from the stream copy */`. The 2 callers (:487, :514) pass `rec`. nd_ntab_run stays (nr_call, nd_pdraw_run use it).
6. Only after the user's yes to the glyph question (the orchestrator tells you): apply NAT5R edit 5 again. msg_draw.c :1864-1865 become `{ uint32_t au[48]; uint32_t* q = au; msg_glyph_emit(&rec, &q); nd_rec_alt(msg_glyph_run, &rec, sizeof rec / 8u, 1, au, (unsigned)(q - au) / 2u, 0); } /* NAT5F */`. The fallback (:1748-1749) becomes `{ static unsigned gfr = ~0u; if (pm_drec_now() && pm_frame_count() != gfr) { gfr = pm_frame_count(); pm_log_evt("glyphalt f%u", gfr); } } nd_alt_run(u + 2u * (unsigned)(sizeof(NdGlyphRec) / 8u)); /* NAT5F */` with `int pm_drec_now(void);` at :1711. Delete msg_glyph_feed and buf, nrp. Without the yes, skip edit 6.
7. nd_mtx_set.c rec_lights (:204-205): the `t[2]` line and `if (!nd_unit_same(t)) nd_unit_apply(t);` become `nd_light_count(NUML(n)); /* NAT5F: set_mw NUMLIGHT (nd_set.c:110); nd_unit_same is 0 for it (:143) */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5f_host.txt 2>&1`: PASS (an lkey census count: fix the count in 3ds/tests/lkey/allow.txt). Commit `NAT5F 1: list units as raw alt ranges; rec_lights`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5f_base_<B>.txt 2>&1`. 3. `sh 3ds/tools/rowloop.sh --row NAT5F --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5F: raw list alt ranges" > build3ds/out/nat5f_loop.txt 2>&1`.
4. `grep -H "glyphalt\|c0fb\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5F_cmp_*.txt > build3ds/out/nat5f_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5F/pm_3ds.elf > build3ds/out/nat5f_c0gate.txt 2>&1`. The base is the start-commit ELF (bench base lane), not build3ds/pm_3ds.elf.
## Gate (exact): host PASS. Rowloop exit 0 (4 drec equal). c0gate calls = base - 2 (base - 3 with edit 6), interp 33, state 23. With edit 6 only: a DIFF bench passes when its log has a `glyphalt` line at or before the DIFF frame (user decision). Fix rights: compile errors, prototypes, the allow.txt count. Any other DIFF, a crash or a tool fault: stop, send nat5f_c0src.txt and the first DIFF line to oracle NATIVE3 (a86ed4bedc3a8575e), wait.
## Hand-back: last commit `NAT5F 2: gate. <gate lines>` with `git add -f build3ds/out/nat5f_c0src.txt build3ds/out/nat5f_loop.txt`. It deletes this page. One SubagentHandback: hash, gate line per bench, c0gate line, glyphalt lines.
