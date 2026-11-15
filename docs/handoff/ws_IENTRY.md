# ws_IENTRY: an inherit list that no seed gives a table gets one unseeded table (gset ok 1075 -> 1211; C5, C6, C7 move in ws_IMOVE)
Model: Haiku (4 exact edits of one tool, 1 new host test). PM_ROW, PM_LANE = IENTRY. Branch IENTRY, worktree build3ds/wt/IENTRY. Budget: 1 build, 6 Azahar runs. Start now.
Owns: 3ds/tools/nd_gfx_conv.c, 3ds/tests/host.d/ientry.sh (new). Edit no other file.
FACT (nd_gfx_conv.c:808-810): the device entry NdNDev of a list holds only its units (the list itself, or ndnu when moved). The device never reads NDN_PSO
(no NdNTab reader in 3ds/src). So a seed changes no device byte. gset_check (:855) needs only a table. Real seeds stay for the walker-free step (oracle_handoff.md).
FACT (oracle dry run of E1 to E3 on main 09607a5033): itab 154, `gconv ntab tables 1488 inherit 72`, triangles 10293, descs 323 (no change), `gconv gset ok 1211 of 1546 lists`
(was 1334, 226, 8258, 1075). nd_gtab_test `ntab tables 1488 draws 10293 bad 0`, `flat ... bad 0`, ND HOST TESTS PASS. The dry run wrote the unsup line of the 72 (unsup 99);
E2 does not, so expect unsup 27 (code read). Line numbers are of main 09607a5033: the quoted text is the anchor.
## Group 1: the converter (0 builds, 0 runs, commit `IENTRY 1: ...`)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ientry_h0.txt 2>&1`. Copy build3ds/out/ntab_entries.txt and ntab_pairs.txt to build3ds/out/ientry_e0.txt and ientry_p0.txt.
2. E1, nd_gfx_conv.c:221. Before `static V2C* cx;` After:
   `static V2C* cx;` and a new next line `static int v2_inh_mode; static unsigned v2_n_itab; static unsigned char v2_itab_t[V2_MAXTAB]; /* IENTRY: the last pass, an unseeded table of an inherit list */`
3. E2, nd_gfx_conv.c:674-675. Before `    if (cx->inh) return -1;` / `    if (cx->unsup[0]) { if (!seed) { fprintf(fu, ...`
   After `    if (cx->inh && !v2_inh_mode) return -1;` / `    if (cx->unsup[0]) { if (!seed && !v2_inh_mode) { fprintf(fu, ...` (rest of the line unchanged).
   In the same function, after the line `    v2_seed_t[v2_n_tab] = seed != NULL;` add `    v2_itab_t[v2_n_tab] = (unsigned char)v2_inh_mode;`.
4. E3, nd_gfx_conv.c:783 (in v2_convert_all, before the loop `for (...) { if (!isinh[i]) continue; if (itxt[i]) fputs(itxt[i], fi);`) insert:
   `    /* IENTRY: an inherit list that no seed gave a table gets one unseeded table (entry 0); the device reads only its units (NdNDev) */`
   `    for (unsigned i = 0; i < gconv_nsym; i++) { if (!isinh[i] || v2_has[i]) continue; v2_inh_mode = 1; const int s = v2_one(&gconv_sym[i], NULL, fu); v2_inh_mode = 0; char ln[400];`
   `        if (s > 0) { v2_n_itab++; snprintf(ln, sizeof ln, "%s: itab (unseeded table, its draws inherit the caller state)\n", gconv_sym[i].name); }`
   `        else snprintf(ln, sizeof ln, "%s: itab unsup %s\n", gconv_sym[i].name, cx->unsup); v2_itx(&itxt[i], ln); }`
5. E4, nd_gfx_conv.c:794. Before `if (!v2_seed_t[i]) fprintf(ft, ...` After `if (!v2_seed_t[i] && !v2_itab_t[i]) fprintf(ft, ...` (codemod.py --ntab and selfapply.py read
   ntab_tables.txt as self-contained tables: an itab table must not be in it). nd_gfx_conv.c:850: add ` itab %u` at the end of the `gconv ntab` format and `, v2_n_itab` as the last argument.
6. New file 3ds/tests/host.d/ientry.sh (LF). Form of 3ds/tests/host.d/iseed.sh. Lines:
   `L=Entity_Signpost_RenderSign`
   `chk() { grep -q "^$L: itab (" "$1" && grep -qx "$L" /tmp/gconv/gset_ok.txt && ! grep -qx "$L" /tmp/gconv/ntab_tables.txt; }`
   `chk /tmp/gconv/ntab_inherit.txt || { echo "ientry FAIL: $L has no itab table, or it is in ntab_tables"; exit 1; }`
   `grep -v "^$L: itab" /tmp/gconv/ntab_inherit.txt > /tmp/ientry_fault.txt; chk /tmp/ientry_fault.txt && { echo "ientry FAIL: seeded fault not seen"; exit 1; }`
   `echo "ientry ok"`
7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ientry_h1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `ientry ok`, `gconv ntab tables 1488 inherit 72 unsup 27 descs 323 triangles 10293 seeded 259 callseed 12 itab 154`,
   `gconv gset ok 1211 of 1546 lists`, `ntab tables 1488 draws 10293 bad 0`.
   `cmp build3ds/out/ntab_entries.txt build3ds/out/ientry_e0.txt` and `cmp build3ds/out/ntab_pairs.txt build3ds/out/ientry_p0.txt` print nothing (proof that psochk input is
   unchanged: psochk runs only from an entry record, nd_walk.c:651-652). Commit.
## Group 2: the gate (1 build, 6 runs, commit `IENTRY 2: ...`)
8. One build: rc 0, `memcheck ... margin` above 0 KB. For B in spot_mac01_drec sbk14_drec fxr_drec: `python 3ds/tools/bench.py base $B --row IENTRY --commit $(git merge-base HEAD main)`,
   `python 3ds/tools/bench.py run $B --row IENTRY --why "IENTRY 2"`, `python 3ds/tools/bench.py cmp $B --row IENTRY --commit $(git merge-base HEAD main)`. Pass: byte-equal (one bit fails).
9. Delete this page. Commit.
Fix rights: a host FAIL that names nd_gtab_test (`FAIL ntab`) or a count other than the FACT: `git checkout -- 3ds/tools/nd_gfx_conv.c`, send build3ds/out/ientry_h1.txt to the oracle.
A cmp difference: send the cmp output path to the oracle (CLAUDE.md 5). Never change a base. A tool fault stops the gate (CLAUDE.md 4).
Stop: group 2 committed and gated. Hand-back build3ds/out/handback_IENTRY.txt, first 8 lines: commit hashes, the gconv ntab and gset ok lines of h0 and h1, the 3 cmp results,
memcheck margin, Azahar runs used.
