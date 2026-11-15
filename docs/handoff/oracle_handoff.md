# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 0db42b31e8, 2026-11-15. 32nd oracle (display list work). A second oracle owns the NDN-pointer reader class (ws_LKEY: nd_lkey.h,
lkey_cen.py census): do not touch those readers. Running: ws_GSETI (gset.py). Ready: ws_LKEY, ws_FXVROM. After Merge FXVROM: ws_FXTYPE.
User order 2026-11-15: BATCH. One page covers a family or a directory (one codemod, one gset set, one gate). Up to 3 coders.
## Counters (FACT, listcen on main 1e45bdcad9 + converter edits only)
C1 build 738 (gbi.h 382, gbi_custom.h 61, gs2dex.h 18; model.c 67, effects 46, world 45, imgfx.c 21, entity 21), C2 gfxpos 224,
C5 data 5149, C6 data_arrays 461, C7 gfxtype 1083, C8 reader 783, C10 recdl 15.
## Dry-run tools (scratch worktrees, nothing on main)
- Script form: scratchpad fxv_dry.sh of oracle 32 (worktree add, edit script, docker gen_gfx_tab + nd_gfx_tab.o + conv2c.sh, seed converter
  with -D, gset.py on a set, second run, listcen). One gconv run is about 4 min. Run it in the background.
- Host-compile a candidate source first: `gcc -m32 -w <DEF> -DND_GCONV_IN <INC> -fsyntax-only <file>` (flags: gen_gfx_tab.sh:11-13).
- Base FACT (1e45bdcad9): gconv lists 1763, tables 1745, inherit 40, unsup 19, gset ok 1660, moved 1578; conv2c ok `ntab tables 1745 draws 30174 bad 0`.
## FXVROM (page ready: ws_FXVROM.md + _code)
- FACT run 1 (split + m_tri rule): tables 1771, inherit 21, unsup 12, gset ok 1693, bad 0; conv2c ok `ntab tables 1771 draws 35637 bad 0`.
  Seed -DROM_SPLIT=0 gives `D_09002CE8_3C0688: vertex ref outside the set` back. gset on 118 sources: moved 33 files 2 keep 0.
- FACT (code read): the device never reads NdNTab (ndno/ndni/ndnv: no reader in 3ds/src). It runs the unit words ndnu (nd_walk.c:599 nd_ntab_run).
  The split changes only the host compare form. The ndnu words and the ndd_ keys do not change. The last oracle's GUESS (2 moved lists draw
  host-layout vertices on the device) is wrong: the device load reads from the array start in ROM-ordered memory (pm_3ds.map 0x4e0a20, +0x70).
## Other families (FACT from oracles 31-32)
- C1/C2: the codemod --ntab dry (main) keeps all dl sites `no-entry`: effects 32, world 36, entity 21, battle 6, screen_overlays 16, curtains 5,
  entity_model 4. They need v2 tables first. Re-run the dry after Merge GSETI: `codemod.py --dry --state --ntab build3ds/out/ntab_tables.txt <dir>`.
- NAMESPACE A(x) (kzn2/actor/lava_piranha.c, sam2/dlist.c): gen_gfx_syms.py:21 and gset.py:32 NS_DEF take `A`. A(sym) = AREA_sym (macros.h:44,
  :200). Their users are readers: lava_piranha `ems_Draw(N(DList1), 60)`, sam2 monstar.c:157 `.displayList = N(dlist)`: wait for LKEY census.
- npc_merlee.c is #included by dro_02/npc.c:53 (not a device source): gset needs an include-twin rule. filemenu_selectlanguage.c is not in the
  device build (3ds/CMakeLists.txt:291-299): drop it.
- pra_31 stairs_skele.c (34 includes, 323 data): animator skeleton nodes; unsup `unit address outside the symbol map` and `matrix address
  outside the symbol map` (Matrix4s N(unk_02_mtx), stairs_vtx.c:120). MESHST-like design.
- model.c (516 data, 169 gfxtype, 67 build): Gfx_RM* lists are NdSDef defs (model.c:1260, nd_rmdef.h), built at run time (nd_rec.c:145 nd_rm_build);
  LKEY H edits nd_rec.c for ModelRenderModes keys. Needs a design (converter emits the compiled defs); Sonnet approved by the user.
- C7 effects: fxtype dry 89 ok arrays; the other 50 are read by a local `Gfx* dlist = X[i]` (next FXTYPE step: local handle rule).
## Tidal Wave (user 2026-11-15, build 5798: flashing blue rectangle, slow). Page ws_GRABSEQ ready (60 FPS cause).
- FACT: Tidal Wave = fx_underwater (sushie.c:336); no src/effects/tidal_wave.c. fxr_drec draws it (fxtest 87) at 30 FPS only.
- OPEN: the 30 FPS console cause (if the user had 60 FPS off), and the console cost (fxcena/b/c 87 lines from the user).
## Older facts that still cost time
- NDN_DECL/NDN (nd_gtab.h:66-74): host form extern Gfx; device form ndd_<name> NdNDev. gset include mode is src/effects/gfx only.
- listcen skips an asset that only a gfxin twin includes. gen_gfx_tab copies its logs to /w/build3ds/out when it exists.
- fxr_drec first run can flake cold (grab_tex hash): warm rerun rule (bench.py:2023).
- Windows python cannot open /c/... paths: pass `cygpath -m` paths.
## What to do first
1. Answer coder questions (GSETI, LKEY, FXVROM) with SendMessage, file:line.
2. After Merge GSETI: re-run the codemod --ntab dry and write the C1/C2 dl page per directory.
3. After Merge LKEY: A(x) namespace + merlee include-twin page; model.c design (Sonnet); pra_31 skeleton design.
