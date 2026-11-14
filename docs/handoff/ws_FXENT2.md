# ws_FXENT2: capture the 42 kept effect sites again with an exact filter, apply the single ones (C10 recdl), list the rest for hand rows
Model: Haiku. PM_ROW, PM_LANE = FXENT2. Budget: 8 Azahar runs. Start after `Merge NSTREAM2` shows in `git log --oneline main` (GKEY2 and NSTREAM2 edit nd_walk.c
and nd_gtab.c). Branch FXENT2. Files: 3ds/src/nd_walk.c (entcap_rec only), 3ds/src/nd_gtab.c (nd_rec_dl no-table branch only), 3ds/tools/entcap.py, src/effects/*.c
(entapply lines, and the join of step 4).
FACT (grep, main 1149b5513): 42 `nd_rec_dl(` lines in src/effects (listcen recdl 57 = 42 + 15 root). FACT (Merge ENTRYFX f46d25215): 13 of them were multi.
## Why the old capture missed sites (code read)
entcap_rec returns for any list with a v2 table (nd_walk.c:506). CONV2C2 gave one device entry per inheriting list with a site pair, so a kept site whose list has a
pair at another site is not logged. A list with no NdGTab never reaches nd_gtab_run (nd_gtab.c `if (!t)` branch writes the G_DL unit): no line at all.
## Group 1: the switch (host check, commit `FXENT2 1: ...`)
1. nd_walk.c: entcap_rec gets a third parameter `uint32_t ent`; Before `    if (nd_ntab_find(t->list)) return; /* a v2 table: not a list without one */`
   After `    if (ent) return; /* FXENT2: a call with a stated entry (nd_draw_list with an ND_PSO_DESC); ND_SELF and nd_rec_dl calls are logged */`.
   `keys[1024]` -> `keys[4096]`, `nkeys == 1024u` -> `nkeys == 4096u`. The call (nd_gtab_run): `entcap_rec(t, rec[2]);` -> `entcap_rec(t, rec[2], rec[3]);` (rec has 4 words when entcap
   is on: nd_gtab.c records r4 with 2 units). Fix the comment above entcap_rec: "a static list call with no stated entry".
2. nd_gtab.c, the `if (!t) {` branch of nd_rec_dl, first line: `if (__builtin_expect(PMDL("entcap"), 0)) entcap_nt(list, (uint32_t)(uintptr_t)__builtin_return_address(0));`
   and above nd_rec_dl: `static __attribute__((cold, noinline)) void entcap_nt(const void* l, uint32_t site) { static uint32_t k[256]; static unsigned n;`
   `const uint32_t a = (uint32_t)(uintptr_t)l; for (unsigned i = 0; i < n; i++) if (k[i] == (a ^ site * 2654435761u)) return; if (n == 256u) return;`
   `k[n++] = a ^ site * 2654435761u; pm_log_evt("entcap nt l %08x s %08x", a, site); }` (pm_port.h, included at nd_gtab.c:85, declares pm_log_evt).
3. entcap.py: `NT = re.compile(r"entcap nt l ([0-9a-f]{8}) s ([0-9a-f]{8})")`; each NT line prints `nt <list symbol> <site function> <file:line>` (same lookup as an l line);
   the last line becomes `entcap.py lists %d sites %d over %d nt %d`. entapply.py needs no edit (a nt line has no 11 words: it counts as other).
4. Join src/effects/recover.c:205-207 (the `nd_rec_dl((type == 0 || type == 2) ?` call over 3 lines) into one line. No other change.
nd_host PASS, one build. Commit.
## Group 2: capture and apply (3 capture runs, then 1 build, gate runs, commit `FXENT2 2: ...`)
5. `bench.py run fxent_a --row FXENT2 --why "FXENT2 capture a"`, same for fxent_b, fxent_c. For each: `python 3ds/tools/entcap.py <log_path of the run> > build3ds/out/entcap_<x>.txt`.
6. `python 3ds/tools/entapply.py build3ds/out/entcap_a.txt build3ds/out/entcap_b.txt build3ds/out/entcap_c.txt > build3ds/out/entapply2.txt`. It edits only `nd_rec_dl` lines with one tuple.
7. nd_host PASS, one build, memcheck margin 310 KB or more. Gate: for each letter with an applied site, `bench.py run fxchk_<x>`: its last psochk line has bad 0. Then
   `bench.py run gold_12_drec` and `bench.py cmp gold_12_drec --row FXENT2 --commit 930de29e1`: byte-equal. listcen recdl falls by the entapply site count exactly.
8. Write build3ds/out/fxent2_rest.txt: one line per `nd_rec_dl(` left in src/effects: `<file:line> <class>`, class = multi (entapply multi line), nt (an entcap nt line with
   that site), seen (an entcap l line for that site, not applied) or unseen (no line for that site). Commit with the entapply total line. The commit deletes this page.
Fix rights: the files above. Never edit a site by hand (step 4 is the only one). A FAIL, psochk bad above 0 or a cmp difference: 2 tries per named cause, then no commit,
build3ds/out/fxent2.txt, send the path to the oracle. Hand back 8 lines: hashes, entapply total, recdl before and after, class counts of fxent2_rest.txt, gate results.
The oracle writes the hand rows (multi: one desc per branch; nt: the list joins a gconv_src.d row; unseen: fxtest variants) from fxent2_rest.txt.
