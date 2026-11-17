# ws_GCONVSPLIT (Haiku): each gconv source compiles alone, and the object cache keeps one entry per header state (counters T1, T2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GCONVSPLIT, PM_ROW=GCONVSPLIT PM_LANE=GCONVSPLIT. Start now, parallel with VOLHOST.
## Objective
- FACT (brief): a header edit that every gconv object reads compiles gconv_in.c again: 96 s, one TU of 168 sources.
- FACT (oracle host run, main folder): `gconvobj: hit 0 miss 80` and 82 `failed its check ... removed` lines. One entry per
  source (gconv_objcache.py:147-155): a lane with other headers removes the entries of main, and main removes them back.
- FACT (oracle runs of this patch with ws_VOLHOST.patch): 9 output files byte-equal to the old outputs in 4 runs. 247 objects.
  nd_tx.h comment: `hit 227 miss 20`, STAGE gfx-table 14 s. nd_gbi.h comment: `hit 2 miss 245`, gfx-table 45 s.
  Probe: all 246 TUs in 39 s at 16 jobs. dgb_01/gfx.c (twin) alone takes 30 s.
- Target: STAGE gfx-table 60 s or less after an edit of a header that all objects read. Also cuts post_merge step 1 (cold gconv).
## Design (the patch does it)
- gen_gfx_syms.py: every source gets its own TU gconv_tu_<path>.c (each character of the path other than A-Z a-z 0-9 becomes _)
  with the array gconv_sym_tu_<path>[]. Two sources with one name stop the tool. A new source does not rename the others.
  Print order: the SHARED sources first, then the others (the old link order, so the gconv_sym section order stays).
- gconv_objcache.py v2: <16 hex of primary key>/<16 hex of sha1(deps.sha1)>/, at most 4 entries per primary key. Each entry
  that a lookup reads is checked (key.txt, out.sha1). A bad one is removed. A hit needs every dep sha1 equal. v1 entries are
  removed at the start. 16 jobs (FACT: fit the 4 GB container), the largest source first.
- Keys. Primary (gconv_objcache.py:54): `gconv-obj v2`, gcc and as versions, flags, TU path, sha1 of the TU file. Entry: + the sha1
  of every file that gcc -MD and as --MD name. gencache gfx_tab (build.sh:150-154) does not change: 3ds/tools is in GT_IN.
## Files (all in docs/handoff/ws_GCONVSPLIT.patch). Edit only these 7: 3ds/tools/gen_gfx_syms.py, gconv_objcache.py, gen_gfx_tab.sh
(comment), 3ds/tests/host.d/gconvw.sh (new text without its 2 raw CR bytes), gdefren.sh, gfxinc.sh, 3ds/tests/gconvw/exp_tu0.c.
Edit shape. Before (gen_gfx_syms.py:177): `path = out_dir / f"gconv_tu{k}.c"`. After: `path = out_dir / f"gconv_{tag}.c"`, `tag = tu_tag(s)`.
Before (exp_tu0.c:4): `static const GSym gconv_sym_tu0[] GCONV_SEC = {`. After: `... gconv_sym_tu_3ds_tests_gconvw_w_c[] ...`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `PM_ROW=GCONVSPLIT PM_LANE=GCONVSPLIT PM_KEY_ONLY=1 sh 3ds/build.sh > build3ds/out/gcs_ref.txt 2>&1`. Then
   `mkdir -p build3ds/out/gcs_ref && cp 3ds/src/nd_gfx_tab.c 3ds/src/nd_ntab_descs.c 3ds/src/nd_pso_tab.c build3ds/out/gcs_ref/`.
2. `rm 3ds/tests/host.d/gconvw.sh`, then `tr -d '\r' < docs/handoff/ws_GCONVSPLIT.patch | git apply -`. A reject: stop.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gcs_host.txt 2>&1`: PASS, `gconvw ok`, `gdefren ok`, `gfxinc ok`.
4. Commit the 7 files: `GCONVSPLIT 1: each gconv source compiles alone, and the object cache keeps one entry per header state`.
5. Step 1 again to build3ds/out/gcs_key.txt: rc 0, `gconvobj: hit 247 miss 0`. `cmp` each file of gcs_ref with 3ds/src/<name>: no output.
6. `PM_CACHE=check PM_ROW=GCONVSPLIT PM_LANE=GCONVSPLIT PM_KEY_ONLY=1 sh 3ds/build.sh > build3ds/out/gcs_check.txt 2>&1`:
   rc 0, `cachecheck gconvobj ok 247 stale 0 new 0`, no `CACHECHECK FAIL`.
7. `PM_ROW=GCONVSPLIT PM_LANE=GCONVSPLIT sh 3ds/build.sh > build3ds/out/gcs_build.txt 2>&1`: rc 0. Then `bench.py run
   spot_mac01_drec --row GCONVSPLIT` (PM_LANE=GCONVSPLIT) and `bench.py cmp spot_mac01_drec --row GCONVSPLIT --commit $(git merge-base HEAD main)`: 1/1 byte-equal.
Budget: 1 host run, 3 key-only runs, 1 build, 1 Azahar start. Seeded faults (FACT, oracle runs): GCONV_OBJ_CAND=1 gives `gfxinc
FAIL a header edit evicted ...`; the clash check off gives `gconvw FAIL TU name clash not seen`.
## Fail. A cmp difference, a CACHECHECK FAIL or a host.d FAIL: 2 tries per cause, then send the output path to the oracle. Do not change
a test. A shell or python error in a patched line: fix that line only, note it in the commit.
## Stop and hand-back. Last commit `GCONVSPLIT 2: gate` deletes ws_GCONVSPLIT.md and ws_GCONVSPLIT.patch. One SubagentHandback:
commits, the gconvobj lines of steps 3, 5, 6, STAGE gen of step 7, the cmp line.
