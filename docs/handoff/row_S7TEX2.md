# Row S7TEX2: texture loads of static lists as build-time binds (NDG_TEX: the tool emits an NdBindDesc, the table runner binds it)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: the tool must model the walker's load sequence (TIMG, load tile, LOADBLOCK or LOADTILE, render tile, TILESIZE, and
  the TLUT group) for each list and decide which groups it can state as one bind; the runner arm must leave the walker texture state
  (nd_ws.td[], tst1, the load model) as the units would, because later units of the same list and S6KEY's key words (tile 0 fmt
  and siz) read it. These are choices in shared code (the tool, the core 0 runner).
- Why not Opus: the bind form exists (NdBindDesc, nd_tex_win_desc, nd_tex_blk_desc, nd_direct_tex: NAPI2B, S5CHUDA); the table
  form and its host compare exist (S7CONV); S7TEX1 made tex_win0 and nd_tex_win_desc linkable on the host.
- Why not smaller: the op, its tool emit, its runner arm and its host compare must change together (nd_host fails otherwise).

## Blocker and base
S7TEX1 and S7RUN merged (nd_texwin.c on the host; nd_gtab_run and nd_rec_dl). `sh 3ds/tools/wt_new.sh S7TEX2`; PM_ROW=S7TEX2
PM_LANE=S7TEX2. Base `git merge-base HEAD main`. Read plan_nativeapi.md section 6 (items 3, 5, 8) and S7RUN's commit first.

## Design (fixed; the coder chooses the inner steps)
1. Op: `NDG_TEX` (next free value in the nd_gtab.h enum), a = the units it covers, p = `const NdGTex*`, with
   `typedef struct NdGTex { NdBindDesc d; const uint32_t* u; } NdGTex;` (u = the first covered unit: the fallback runs the units).
   Name `ndgx_<SYM>_<k0>`.
2. Tool (3ds/tools/nd_gfx_conv.c): a load group is the unit run of one gDPLoadTextureBlock / gDPLoadTextureTile (and _4b) expansion
   that ends with the render tile's TILESIZE, optionally preceded by one gDPLoadTLUT group. The tool emits NDG_TEX only when:
   every image and palette pointer is mapped (sym_of), the render tile is tile 0, and nd_tex_win_desc (tile loads) or
   nd_tex_blk_desc (block loads) returns 1 on the decoded fields. Else the units stay as they are (defs and NDG_UNIT). NdBindDesc
   pointer fields (src, pal) are written as symbol + offset, like NDG_VTX. The tool links 3ds/src/nd_texwin.c (gen_gfx_tab.sh
   compile line; a host stub for nuGfxZBuffer if the file needs it: tool data never aliases the z buffer).
3. Runner (nd_walk.c, nd_gtab_run): `case NDG_TEX: if (!nd_direct_tex(&x->d)) nd_walk_state(x->u, op->a);`. Before that, compare
   nd_direct_tex's writes with the walker's arms of the covered units (nd_set_timg, nd_set_tile, nd_set_loadblock/loadtile,
   nd_set_loadtlut, nd_set_tilesize: 3ds/src/nd_tex_set.c): every field those arms write and a later reader uses (td[0], tst1 tile 0
   fmt, siz, wrap, the load model ld_*, the TLUT state, the tile 7 descriptor) must be written the same way by the arm. Missing
   writes go into a new helper next to nd_direct_tex (nd_direct.c end), not into nd_direct_tex itself (its other callers stay).
4. Debug check: switch `gtexchk` (pm_debug_has): the arm runs the units on the walker after the bind, from a snapshot taken before,
   and compares the texture fields named in 3 (s6chk form, nd_scomp.c:224-287); one line per 100 frames `gtex f<n> tex <t> bad <b>`
   (pm_log_evt; counts since boot).
5. Host compare (3ds/tools/nd_gtab_test.c): for each NDG_TEX op, (a) the covered units are one load group as the rule in 2 reads it
   (an independent decode in the test), (b) the NdBindDesc equals nd_tex_win_desc / nd_tex_blk_desc on that decode, (c) the round
   trip covers every unit once. Prints `gtab ... tex <n>`; any difference fails nd_host.

## Readers and writers of the changed format
- Writers: nd_gfx_conv.c (emit). Readers: nd_gtab_run (S7RUN), nd_gtab_test.c (round trip and the new compare), nd_part_draw
  (nd_walk.c / nd_direct.c: reads VTX and TRI only; it must skip or refuse NDG_TEX like NDG_DEF: check its switch default).
  grep `NDG_` in 3ds before the commit and list each reader in the commit message.

## Files and places
nd_gtab.h (enum end, NdGTex), nd_gfx_conv.c, gen_gfx_tab.sh (the tool compile line only), nd_gtab_test.c, nd_walk.c (nd_gtab_run's
switch only), nd_direct.c (end: the helper), nd_host_test.sh (nd_gtab_test link line if nd_texwin.c is needed). S7EFX rows edit
gen_gfx_tab.sh GCONV_SRC lines only and the effect files: no overlap.

## Gate (expected values)
- Build, memcheck OK. nd_host PASS with `gtab ... bad 0` and `tex` above 0 (hand back the line).
- New scenario and bench `gtexchk_63` (one commit; inserted right after fxpic_63 in azt_scenarios.json and bench_defs.json):
  fxpic_63's debug plus `gtexchk` without `shots shotwin=412:4`, until `gtex f\d+ tex [1-9]\d* bad \d+` (the first line with a bind: the runner matches the first hit), cap 30,
  deadline 20, expected_events `warp: mac_01 entry 0` only; bench kind azahar_pic, speed normal, tags azahar. Log check only (no
  base, no cmp).
- `bench.py base fxpic_63`, `bench.py base fxpic_124`; one list `bench.py run fxpic_63,fxpic_124,gtexchk_63 --row S7TEX2 --why
  "S7TEX2: table binds"` (3 starts): fxpic shots byte-equal; gtexchk_63: `bad 0` and `tex` above 0. Budget 8.

## Stop
A byte difference, a host compare difference, or `gtex ... bad` above 0: stop, commit, hand back (2 tries per named cause).
Delete this row file in the completing commit. Hand back the NDG_TEX count per GCONV_SRC group (the S7EFX rows report theirs).
