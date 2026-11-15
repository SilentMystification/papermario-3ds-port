# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 1df64777c0, 2026-11-15. 31st oracle (display list work; a second oracle hunts the build 5774 map-change crash: do not touch pm_mapdata.c,
pm_map.c map restore, heap.c, nd_mesh.h). Running: ws_PIPEFX (imgfx.c), ws_DLCRD (nd_shape.c, pm_map.c, nd_dlc.c). Ready: ws_GSETI (+ ws_GSETI_code.md).
User order 2026-11-15: BATCH. One page covers a family or a directory (one codemod, one gset set, one gate). Up to 3 coders.
## Counters (FACT, listcen in the main folder, 1df64777c0)
C1 build 738 (461 gbi headers, phase E), C2 gfxpos 224, C5 data 5149, C6 data_arrays 461, C7 gfxtype 1085, C8 reader 779, C10 recdl 15.
(The orchestrator quoted data 5120, gfxtype 947: other run place. Re-run listcen in the main folder before a page.)
C5 by place: assets/us 2767 (effects/gfx 1468 in 95 lists, pra_31 323, starpoint 238, kzn2 138, tst_13 114, hammer 76, dgb_01 72, level_up 54),
src/model.c 516, include 384, src/entity 170, src/world 165, screen_overlays 123, curtains 121, PAL level_up_letters 291, battle 93, pause 72, filemenu 70.
## Dry-run tools (scratch worktrees, nothing on main)
- `git worktree add --detach <scratch>/dry HEAD`, then gen_gfx_tab in docker like build.sh:94-111 (mount the worktree as /w, assets of the main
  folder read-only). Run nd_gtab_test and 3ds/tests/host.d/conv2c.sh in the same container for the host compare. One run is 4-5 min: run it in the background.
- Host-compile a candidate source first: `gcc -m32 -w <DEF> -DND_GCONV_IN <INC> -fsyntax-only <file>` (flags: gen_gfx_tab.sh:11-13). It saves runs.
- Base FACT (1df64777c0): gconv lists 1763, tables 1745, inherit 40, unsup 19, gset ok 1660, moved 1578; conv2c ok run `ntab tables 1745 draws 30174 bad 0`.
## Design FXVROM (next page; dry run done, one host-compare step open)
- FACT: 93 of the 103 not-ok lists are effect lists. The cause is a call-edge chain from 19 unsup lists (gset_check, nd_gfx_conv.c:889-901).
- FACT: the main unsup cause is a gsSPVertex that reads past its array end (lil_oink: `&D_09000080_3FF2D0[0], 5` on a [2] array). The device
  layout is ROM order (pm_3ds_5774.map: D_09000080_3FF2D0 0x50bbf0, D_090000A0_3FF2F0 0x50bc10). The host layout is not.
- Rule (tested in a scratch copy): at nd_gfx_conv.c:539 split a load past its array end. The next piece reads the Vtx symbol whose name ROM hex
  (D_..._<hex>) equals hex + 16 * length. No such symbol: the load stays whole (else 4 moved lists lose their table: gconv FAIL).
  FACT result: tables 1771, inherit 21, unsup 12, gset ok 1693 (+33), gconv bad 0. Then gset.py on all 118 src/effects/gfx sources:
  gset moved 33, listcen data 5149 -> 4233 (-916), data_arrays -33, gfxtype -4; 2nd gconv run moved 1611, bad 0.
- OPEN before the page: conv2c then reports `bad 3732` (FAIL triangle vertex triple: D_09001A00_3E4360, D_09001A20_3E4380, both already moved).
  Cause: nd_gtab_test.c m_tri (:380-381) keeps (array, index past end), the v2 table has (next array, index). Fix design: move vtx_len, rom_of,
  vtx_rom_next into nd_gconv.h (static inline), and normalise e->base[j]/e->i[j] in m_tri by the same rule. Dry run it: pass = ok-run bad 0.
- Expected picture: the 2 already-moved lists drew host-layout vertices on the device (their tables came from the host layout). FXVROM corrects
  them. A drec diff at their draws is a correction: the user decides (one bit differs = gate fails).
- -fno-toplevel-reorder on the gen_gfx_tab.sh compiles changes nothing (FACT, same counts). Do not use it.
## Other families (FACT from this session)
- pra_31 stairs_skele.c (34 includes, 323 data): animator skeleton nodes (`.displayList = &N(unk_01_gfx)`, :201); its lists are unsup
  `unit address outside the symbol map` and `matrix address outside the symbol map` (Matrix4s N(unk_02_mtx), stairs_vtx.c:120). MESHST-like design.
- NAMESPACE `A(x)` (battle actors: kzn2/actor/lava_piranha.c:15, sam2/dlist.c:3): gen_gfx_syms.py NS_DEF takes `A`. Fix: expand A(x) like N(x).
- Host compile fails: dro_02/npc_merlee.c (EffectInstance type), filemenu_selectlanguage.c.
- model.c (516 data, 169 gfxtype, 67 build): Gfx_RM* lists are NdSDef defs (model.c:1260, nd_rmdef.h), built at run time (nd_rec.c:145 nd_rm_build);
  SolidCombineModes/AlphaTestCombineModes are combine words (model.c:1616, nd_recs.c:86). Readers: animator.c:43-56/785-828, entity_model.c:29+.
  Needs a design (converter emits the compiled defs); Sonnet design step.
- C7 effects (275 Gfx words): `Gfx* X[] = { NDN(..) }` 120+, `Gfx* dlist` 40, `extern Gfx X[]` 40 (unmoved). A typedef handle codemod
  touches the same lines as FXVROM's gset rewrite: start it after Merge FXVROM. Device build uses -w (CMakeLists.txt:324).
## Older facts that still cost time
- NDN_DECL/NDN (nd_gtab.h:64-71): host form extern Gfx; device form ndd_<name> NdNDev. gset include mode is src/effects/gfx only (gset.py:325).
- listcen skips an asset that only a gfxin twin includes (listcen.py:250-259). gen_gfx_tab copies its logs to /w/build3ds/out when it exists.
- OPEN crash 5774: logic thread, reset_player_status 7B440.c:123 reads mac_01 entryList; writer not found (other oracle).
- fxr_drec first run can flake cold (grab_tex hash, pm_gpu.c:2565, :666): warm rerun rule (bench.py:2023).
- NOT NOW (user): pm_mapdata.c:112-123 save malloc; psochk 16 of 20 fields; perf round (nd_ntab_find probes, audit_hw3 F1).
## What to do first
1. Answer GSETI, PIPEFX, DLCRD coder questions (SendMessage, file:line).
2. FXVROM: dry-run the nd_gtab_test normalisation, then write ws_FXVROM (Haiku; nd_gfx_conv.c, nd_gconv.h, nd_gtab_test.c, a vrom.sh seeded test,
   gset.d/FXVROM.txt = the 118 src/effects/gfx sources). Gate: host conv2c bad 0, 4 drec benches; a diff goes to the user.
3. Then: FXTYPE codemod (after Merge FXVROM), A(x) namespace in gen_gfx_syms.py, model.c design, pra_31 skeleton.
