# ws_ISEED: seeds for the inherit lists (the converter `inherit` count falls; no C1..C12 counter moves)
Model: Sonnet (user, 2026-11-15). Why not Haiku: step 3 judges captured C-site states per site, and step 5 writes a converter pass from a spec. PM_ROW, PM_LANE = ISEED.
Branch ISEED, worktree build3ds/wt/ISEED. Budget: 2 builds of the head, 8 Azahar starts. Group 1 starts now (main 59cd90449). Group 2 starts after `Merge LIT1` on main: then run `git merge main` once.
Owns: the src/effects files of steps 1-3, 3ds/tools/nd_gfx_conv.c (group 2 only), 3ds/tools/azt.d/ISEED.json, 3ds/tools/bench.d/ISEED.json, 3ds/tests/host.d/iseed.sh (all new). Edit no other file.
Never run codemod.py on src/effects (it gives one desc of the file to every list call). Use entcap.py and entapply.py as they are.
FACT (nd_host, main 59cd90449): `gconv ntab tables 1192 inherit 260 unsup 24 descs 317 triangles 7168 seeded 227`. Classes of the 260 (FACT, oracle scan of ntab_inherit.txt and src on 59cd90449):
effects 171 = list callees 54, C sites 41, seeded but unsup 35 (vertex slots 24, MOVEWORD 9, other 2), no reference 41. entity 69 = list callees 24, entity-path refs 45. other 20.
This page does the effect C sites (group 1) and the list callees of effects and entity (group 2). Not in this page: entity-path refs, other 20, unsup 35, no reference 41. GUESS: inherit falls by 30 to 90.
## Group 1: effect C-site seeds (1 build, 1 Azahar start, commit `ISEED 1: ...`)
1. Rewrite the 4 gMainGfxPos calls of inherit lists to nd_rec_dl (gt_run logs only nd_rec_dl and nd_draw_list calls, nd_walk.c:619; gSPDisplayList bypasses it): bombette_breaking.c:235, :249,
   shimmer_wave.c:223, stars_shimmer.c:361. Example bombette_breaking.c:235. Before `        gSPDisplayList(gMainGfxPos++, D_E0084E1C[type]);` After `        nd_rec_dl(D_E0084E1C[type]);`. The 3 files include nd_gtab.h (FACT grep).
2. New azt.d/ISEED.json scenario `iseed_cap` and bench.d/ISEED.json bench `iseed_cap`: copy fxent_a (azt.d/ENTRYFX.json:4-20, bench.d/ENTRYFX.json fxent_a). Set `fxtest=` to the fx_names ids (3ds/src/pm_fxtest.c)
   of bombette_breaking, shimmer_wave, stars_shimmer, firework, firework_rocket, chomp_drop, disable_x, explosion, embers, shape_spell. Keep `entcap`. Add scenario `iseed_chk` and bench `iseed_chk`: the same ids, `psochk` in place of `entcap` (the form of fxr_chk, bench.d/FXENT3.json:18-31).
3. Build. `python 3ds/tools/bench.py run iseed_cap --row ISEED --why "ISEED 1 capture"`. `python 3ds/tools/entcap.py <log> > build3ds/out/iseed_cap.txt`. `python 3ds/tools/entapply.py --dry build3ds/out/iseed_cap.txt`, then without --dry.
   Judge each site that entapply does not edit (`skip`, `multi`, ND_SELF lines such as firework.c:293 and firework_rocket.c:277): one tuple per site -> edit by hand to `nd_draw_list(<arg>, pe_<stem>_<line>)` and add the
   ND_PSO_DESC line in the entapply form (entapply.py desc_line). Two or more tuples -> no edit; write the site to the hand-back. A site that keeps a per-draw key in code goes to the hand-back too (a keyed recapture is not in the budget).
   Commit the cap file name and the entapply last line in the commit text.
## Group 2: call-unit seeds in the converter (0 builds, 0 starts, commit `ISEED 2: ...`)
4. New 3ds/tests/host.d/iseed.sh (LF): `grep -q "^D_09000F88_327398 D_09000F88_327398_e" /tmp/gconv/ntab_pairs.txt || { echo "iseed FAIL: D_09000F88_327398 has no call seed"; exit 1; }` then `echo "iseed ok"`.
   Seeded check: `sh 3ds/tools/nd_host_test.sh > build3ds/out/iseed_h0.txt 2>&1` must hold the FAIL line (FACT: its 7 callers D_09000FA0_3273B0 .. D_09001420_327830 have tables, ntab_tables.txt). Else stop (CLAUDE.md 5).
5. nd_gfx_conv.c. (a) V2C: add `struct { unsigned sym; NdPsoDesc d; } cl[256]; unsigned ncl;`. (b) v2_walk `case NDO_CALL:` after the `if (!cs || off % 8u ...` line: when `off == 0`, `s->has_comb`, `s->has_tex` and
   `(s->omh_set & (3u << G_MDSFT_CYCLETYPE)) == (3u << G_MDSFT_CYCLETYPE)`, add `{ cs - gconv_sym, v2_make_desc(s) }` to cx->cl (full: v2_unsup("too many call seeds", k)). (c) v2_one: after `v2_emit_table(cx);` copy cx->cl to
   new `static V2Pair v2_cp[4096]; static unsigned v2_ncp;` (list = gconv_sym[sym].name; exit 1 when full) and set `v2_has[g - gconv_sym] = 1` (new `static unsigned char v2_has[GS_MAXSYM]`). A walk that returns 0 or -1 adds no pair.
   (d) v2_convert_all: keep the first loop, but store the `inherits` text per symbol in place of fprintf and `v2_n_inh++`. Then loop until a pass makes no table: for every GS_GFX symbol with !v2_has, try each call pair of
   it not tried before (tried ids per symbol, as the first loop does) with v2_one(g, &d, fu). Then write the `inherits` line and count v2_n_inh for each symbol that is still !v2_has. (e) The `gconv ntab` stderr line:
   append ` callseed %u` (new `static unsigned v2_n_cseed`: tables made in the loop). Before (main) `... seeded %u\n", v2_n_tab, ...` After `... seeded %u callseed %u\n", v2_n_tab, ..., v2_n_seed, v2_n_cseed`.
6. `sh 3ds/tools/nd_host_test.sh > build3ds/out/iseed_h1.txt 2>&1`: `ND HOST TESTS PASS`, `iseed ok`, `bad 0`. Write the h0 and h1 `gconv ntab` lines to the commit text. Commit.
## Gate (1 build, 6 Azahar starts, commit `ISEED 3: ...`)
7. Build rc 0, `memcheck ... margin` above 0 KB. For B = iseed_chk, fxr_drec, spot_mac01_drec: `python 3ds/tools/bench.py base B --row ISEED --commit $(git merge-base HEAD main)`, then
   `python 3ds/tools/bench.py run B --row ISEED --why "ISEED gate"`. iseed_chk: the `psochk end` line of the head has the same `bad` value as the base (the B1 rule of FXREST2). fxr_drec, spot_mac01_drec:
   `python 3ds/tools/bench.py cmp B --row ISEED --commit $(git merge-base HEAD main)` byte-equal. Delete this page. Commit.
Fix rights: a psochk bad rise names a site (`psochk bad` line): take that site back to its main text (2 tries per site). A cmp difference or a host FAIL: 2 tries per named cause, then the stop rule (CLAUDE.md 5).
Stop: gate passed and committed. Hand-back build3ds/out/handback_ISEED.txt, first 8 lines: commit hashes, h0 and h1 `gconv ntab` lines, entapply last line, iseed_chk bad base and head, the 2 cmp results,
memcheck margin, sites left (multi, keyed, skip) with file:line.
