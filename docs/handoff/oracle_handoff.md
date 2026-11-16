# Oracle handoff (edit in place; a fresh oracle reads this, then CLAUDE.md and .claude/agents/oracle.md)

State: main 2e0746e540, 2026-11-16. Oracle 36 wrote ws_MAPLAG (+ _code, ready). Earlier pages ready: ws_STARTLR2, ws_MOON7, ws_FSKIP.
OPEN for the next oracle: WAVEWIDE (Tidal Wave blue square in wide), trace in the Tidal Wave section.
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
## Tidal Wave in wide (user 2026-11-16, build 5852, 60 FPS on: blue flickering square in the 4:3 window). GRABSEQ, GRABFL merged.
- FACT: the mesh is 4:3 by design: underwater.c:183-194 game x 12..308, y 20..220, ortho 0..1280 (:236), tc = the same texels. It never
  touches game x 0 or 320, so wide_edge_k (pm_gpu.c:3406) never moves it; as ui_ortho it lands at screen 40 + x (pm_gpu.c:229-230): 52..348.
- FACT: the wide grab is consistent: 400 columns (pm_gpu.c:2775), slot at cfb - 80 (:2245), origin 40 (pm_tex.c:1719), tile_x0 -40
  (nd_tex_set.c:420): mesh texel s reads screen column s + 40, the same column it covers.
- FACT: PM_CC_48 (macros.h:501) = PRIM * PRIM_A + TEXEL0: solid blue means TEXEL0 = 0 (empty or wrong slot) in those frames.
- FACT: hw_tidal1_live.txt has no `tidal:` line; its grab/fbbind lines (#176540-#176579) come with grabw type 2 = gut_tex (pm_gpu.c:1447),
  the pause screen (background_gfx.c:374), not the wave.
- FACT: the two cfbs are adjacent (0xff7e10 + 153600 = 0x101d610) and a slot spans 256 rows (pm_tex.h:6): pm_tex.c:1715 matches a load of
  the upper cfb to the lower cfb's slot (off_oy 240, empty rows) when both cfb slots live in one frame. Not shown for the wave.
- Next: read the walker tile clamp for a slot (nd_tex_set.c, nd_geom uv) before a design that widens the mesh to game x -40..360 in wide
  (non-wide byte-equal). Ask the user for a wide wave capture with grablog fbbind (CAPTOOL capture_check first).
## Older facts that still cost time
- NDN_DECL/NDN (nd_gtab.h:66-74): host form extern Gfx; device form ndd_<name> NdNDev. gset include mode is src/effects/gfx only.
- listcen skips an asset that only a gfxin twin includes. gen_gfx_tab copies its logs to /w/build3ds/out when it exists.
- fxr_drec first run can flake cold (grab_tex hash): warm rerun rule (bench.py:2023).
- Windows python cannot open /c/... paths: pass `cygpath -m` paths.
## Console log capture (oracle 35, 2026-11-15). Pages ws_CAPLOG, ws_CAPTOOL ready (parallel, disjoint files).
- FACT: with log_file_on 0 and prof_mode 0 the writer never calls accept (pm_log.c:1040-1043, :1066); options.ini log_on=0 does it (pm_options.c:229);
  the log profile had no log=on. GUESS: the O3DS XL options.ini has log_on=0 (menu Profiling row saves it, pm_options.c:269).
- FACT: with logging off, event lines (pm_log_evt) reach no log.txt: no writer wake (pm_log.c:600), sink waits (:888). Not fixed yet.
## What to do first
1. Answer MAPLAG coder questions (SendMessage, file:line).
2. WAVEWIDE: finish the trace above, then write ws_WAVEWIDE (+ _code).
