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
S7TEX1, S7RUN and S7ENT merged (S7ENT adds ent_tdwalk table pieces, which must know NDG_TEX). `sh 3ds/tools/wt_new.sh S7TEX2`;
PM_ROW=S7TEX2 PM_LANE=S7TEX2. Base `git merge-base HEAD main`. Read plan_nativeapi.md section 6 (items 3, 5, 8) first.

## Try 2 contract (ORACLE7, after the coder's read of try 1's item 3; row spec fault, no budget used)
Facts (code read, main 87a256ef5): nd_direct_tex (nd_direct.c:142-151) returns nd_tex_bind_desc's result, 1 = the layer held the
image, 0 = re-windowed or loaded: both are a finished bind, there is no refusal. It writes only tile_x0/y0/fx/fy, tsz_last and the
bind. The covered units write much more through the setters of nd_tex_set.c: set_timg :48 (tst1 timg fields, ld_valid 0, the cfb
capture), set_tile :60 (tst1 tile fields; tile 0: flush, pm_tex_set_wrap, st0_ep), set_loadblock :90 / set_loadtile :107 (blk and
ldrec records, the ld_* load model), set_loadtlut :122 (tlut, tlut_n, tlut_pal, tlut_v20), set_tilesize :140 (tile_sz, td[0], the
cur_tile -1 lazy bind, dual_on and sync_combine, the m1 shaded arm, the glyph and tsz_last memo arms). ensure_tile_slow :335 adds
the fb_prev_base and glyph_atlas_bound arms at the first draw. Later units, S6KEY's key words and ensure_tile read that state.
Chosen contract: option (b), the walker state stays the producer of the bind until S9DEL2. NDG_TEX is the build-time form of one
load group: its units run with the walker's own setters and the bind stays lazy (ensure_tile at the first draw), so the bytes are
equal by construction. The NdBindDesc is the build-time answer of that bind; the debug check proves it equals the walker's td[0]
and tile 0 fields, so S9DEL2 can later bind it directly and delete the tile model. Option (a) (bind at the group) is rejected: it
moves the bind before the draw, skips the fb_prev_base and glyph arms and changes flush order.

## Design (fixed; the coder chooses the inner steps)
1. Op: `NDG_TEX` (next free value in the nd_gtab.h enum), a = the units it covers (1 to 32), p = `const NdGTex*` with
   `typedef struct NdGTex { NdBindDesc d; const uint32_t* u; } NdGTex;` (u = the first covered unit, in place in the list).
   Name `ndgx_<SYM>_<k0>`.
2. Tool (nd_gfx_conv.c conv()): before the def search at unit k, test for a load group starting at k: the unit run of one
   gDPLoadTextureBlock / gDPLoadTextureTile (and _4b) expansion that ends with the render tile's TILESIZE, optionally preceded by
   one gDPLoadTLUT group; sync and no-op units inside count as covered. Emit NDG_TEX only when every image and palette pointer is
   mapped (sym_of), the render tile is tile 0, a CI image (fmt 2) has its own TLUT group in the run, and nd_tex_win_desc (tile
   loads) or nd_tex_blk_desc (block loads) returns 1 on the decoded fields. The def search never extends into a unit that starts a
   load group (cap n there). Else the units stay as today. NdBindDesc src and pal are written as symbol + offset (as NDG_VTX). The
   tool links 3ds/src/nd_texwin.c (gen_gfx_tab.sh tool compile line). No stub (user rule 2026-11-12): nd_tex_win_desc reads
   nuGfxZBuffer (nd_texwin.c:74, the one-row z-buffer source rule; nuGfxZBuffer is defined in pm_main.c). Split it for real:
   nd_texwin.c gets `nd_tex_win_core(...)` = nd_tex_win_desc without the z-buffer test (pure), and nd_tex_win_desc moves to
   nd_direct.c as `core(...) && !(o->h <= 1 && <the same z-buffer test>)`, so its device callers keep the same result. The same
   for blocks: nd_texwin.c gets `nd_tex_blk_core` (nd_tex_blk_desc's body on nd_tex_win_core), and nd_tex_blk_desc stays in
   nd_direct.c as blk_core plus the same z-buffer test. The tool and nd_gtab_test call the two core functions; the device
   callers keep the desc functions with unchanged results. A table with NDG_TEX
   is never NDG_F_PARTS (the parts rule at nd_gfx_conv.c:121-131 already sets parts 0 for any other op: keep it so).
3. Runner (nd_walk.c, next to gt_ops): `static void gt_tex(const NdGTex* x, unsigned n)`: for each of the n units from x->u: a sync
   or no-op unit is skipped (as op_typed's caller skips it, before the prologue); else the op prologue and the setter exactly as
   op_typed (nd_walk.c:255-265) without its debug hooks: `pm_gbi_cur_op = nd_op_n64[id - NDO_BASE]; gv_epoch++; nd_op_apply(u);`
   (nd_op_same returns 0 for texture ops: nd_set.c:122-145, no case). gt_ops: `case NDG_TEX: gt_tex((const NdGTex*)o->p, o->a);
   break;` before `default`. No bind call in the arm.
4. ent_tdwalk (nd_ent.c, S7ENT): an NDG_TEX op is a piece of kind UNIT with n = op->a and p = x->u (nd_ent_pieces then runs
   nd_walk_state(p, n): check that its UNIT kind takes n, else add the n). Tiling counts its units like any piece.
5. Debug check, switch `gtexchk` (pm_debug_has, read once): after gt_tex, when nd_ws.td[0].valid, compare d.src, pal, pal_n, fmt,
   siz, w, h, stride, x0, y0 with td[0] and d.wrap, d.sz with tst1.tile_wrap[0] and tile_sz[0]; count tex, bad and skip (td[0]
   not valid: the m1 shaded arm or the z-buffer row arm). One entry per 100 frames `gtex f<n> tex <t> bad <b> skip <s>`
   (pm_log_evt; counts since boot); the first 8 bad ones also print the symbol address and the differing field.
6. Host compare (nd_gtab_test.c): for each NDG_TEX op, (a) the covered units are one load group by an independent decode in the
   test, (b) the NdBindDesc equals nd_tex_win_desc / nd_tex_blk_desc on that decode, (c) every unit of the list is covered once.
   Prints `gtab ... tex <n>`; any difference fails nd_host.

## Readers and writers of the changed format
- Writers: nd_gfx_conv.c. Readers: gt_ops (nd_walk.c), ent_tdwalk and nd_ent_pieces (nd_ent.c, nd_walk.c), nd_gtab_test.c,
  nd_part_draw (nd_geom.c:354: parts tables never hold NDG_TEX, by the parts rule; add no arm, name it in the commit). grep
  `NDG_` in 3ds before the commit and list each reader in the commit message.

## Files and places
nd_gtab.h (enum end, NdGTex), nd_gfx_conv.c (conv(), output), gen_gfx_tab.sh (the tool compile line only), nd_gtab_test.c,
nd_walk.c (gt_tex and the gt_ops case), nd_ent.c (ent_tdwalk), nd_host_test.sh (the nd_gtab_test link line if nd_texwin.c is
needed). Not nd_direct.c. S7EFX and S7FLAME rows edit GCONV_SRC lines, gen_gfx_syms.py main() and effect files: no overlap.

## Gate (expected values)
- Build, memcheck OK. nd_host PASS with `gtab ... bad 0` and `tex` above 0 (hand back the line).
- New scenario and bench `gtexchk_63` (one commit; right after fxpic_63 in azt_scenarios.json and bench_defs.json): fxpic_63's
  debug plus `gtexchk` without `shots shotwin=412:4`, until `gtex f\d+ tex [1-9]\d* bad \d+` (the first entry with a group: the
  runner matches the first hit), cap 30, deadline 20, expected_events `warp: mac_01 entry 0` only; kind azahar_pic, speed normal,
  tags azahar. Log check only (no base, no cmp).
- `bench.py base fxpic_63`, `bench.py base ent_kmr03_det`; one list `bench.py run fxpic_63,ent_kmr03_det,gtexchk_63 --row S7TEX2
  --why "S7TEX2: table texture groups"` (3 starts): fxpic_63 and ent_kmr03_det byte-equal; gtexchk_63 `bad 0`, `tex` above 0
  (hand back skip). Budget 8 (7 planned).

## Stop
A byte difference, a host compare difference, or `gtex ... bad` above 0: stop, commit, hand back (2 tries per named cause).
Delete this row file in the completing commit. Hand back the NDG_TEX count per GCONV_SRC group (the S7EFX rows report theirs).
