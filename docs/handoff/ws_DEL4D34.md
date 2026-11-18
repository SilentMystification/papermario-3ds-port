# ws_DEL4D34 (Haiku): the SDK headers lose their unused Gfx declarations; gt.h goes (W14 DEL4D3 and DEL4D4, early; counters: gfxtype 246 -> 230, build 308 -> 306)
Designer: oracle FIN. Branch from main (no blocker: no running row edits these files). Worktree build3ds/wt/DEL4D34, branch DEL4D34. PM_ROW=DEL4D34 PM_LANE=DEL4D34.
Edit only: include/PR/gt.h (delete), include/PR/gs2dex.h, include/PR/sp.h, include/PR/gu.h, 3ds/tools/gen_nolist.py (1 line), 3ds/tools/gbiprune.py (2 lines).
## Facts (code read, main 4abca1ff8a; listcen on main)
- listcen file counts: gfxtype gt.h 7, gs2dex.h 2, sp.h 5, gu.h 2 (sum 16); build gt.h 2. Totals: gfxtype 246, build 308.
- No file includes gt.h (grep `gt\.h` in src, include, 3ds: only the comment lines of gt.h itself). ultra64.h:33, :35 include gu.h and sp.h.
- No file outside include/PR uses gtStateSetOthermode, guS2D2EmuBgRect1Cyc, guS2DEmuBgRect1Cyc, spDraw, spInit, spFinish, guLoadTextureBlockMipMap,
  guDPLoadTextureTile, rsp_dl, rsp_dl_next (grep -w in src, include, 3ds/src, 3ds/include, 3ds/tools). The game sprite code uses src/sprite.h, not the Sprite of sp.h.
- gen_nolist.py:17-23 opens each header of HEADERS without a test: a missing gt.h stops it. gbiprune.py:62-65 skips a missing header.
## Edits (one shape: delete the declaration lines)
1. `git rm include/PR/gt.h`.
2. gs2dex.h: delete :339 `  extern void	guS2D2EmuBgRect1Cyc(Gfx **, uObjBg *);` and :342 `  extern void	guS2DEmuBgRect1Cyc(Gfx **, uObjBg *);`. Keep the #define lines and the #ifdef frame.
   Before (:337-339): `# define guS2DEmuSetScissor	guS2D2EmuSetScissor	/*Wrapper*/` / `  extern void	guS2D2EmuSetScissor(u32, u32, u32, u32, u8);` / `  extern void	guS2D2EmuBgRect1Cyc(Gfx **, uObjBg *);`
   After: the first two lines only.
3. sp.h: delete :93 (`Gfx	*rsp_dl;`), :95 (`Gfx	*rsp_dl_next;`), :187 (`Gfx *spDraw (Sprite *sp);`), :188 (`void spInit( Gfx **glistp );`), :190 (`void spFinish( Gfx **glistp );`).
4. gu.h: delete :72-76 (the 5 lines of `extern int guLoadTextureBlockMipMap(Gfx **glist, ...`) and :79-86 (the 8 lines of `extern void 	guDPLoadTextureTile (Gfx *glistp, ...`). Keep :78 guGetDPLoadTextureTileSz.
5. gen_nolist.py: delete :21 `    "include/PR/gt.h",`. gbiprune.py: in :14 and in the docstring :2 delete the name include/PR/gt.h (with its comma).
6. Run `grep -rnwE '["/<]gt\.h|rsp_dl|rsp_dl_next|spDraw|spInit|spFinish|guLoadTextureBlockMipMap|guDPLoadTextureTile' src include 3ds/src 3ds/include 3ds/tools --include=*.c --include=*.h --include=*.py`.
   Expect (FACT, grep on main) only: gbi.h:3259 (a macro body, not expanded by any file) and the 6 #define lines of sp.h :165-174. Another hit: stop, send it to the oracle.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-6. `sh 3ds/build.sh` rc 0 (the game and the gconv step compile: the proof that no user is left). `sh 3ds/tools/nd_host_test.sh` PASS.
2. `python 3ds/tools/listcen.py > build3ds/out/DEL4D34_listcen.txt`, the same on the merge-base. Pass: gfxtype 246 -> 230, build 308 -> 306, every other `listcen total` line equal.
   Commit `DEL4D34 1: SDK headers without unused Gfx declarations; gt.h deleted`.
3. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row DEL4D34 --bench spot_mac01_drec: LOOP PASS, byte-equal.
Fix rights: the listed files; compile errors that name a deleted declaration (restore that one line, note it in the commit). A DIFF, another total that changes: stop, send the output path to the oracle. Wait.
Stop: last commit `DEL4D34 2: gate. spot_mac01_drec byte-equal; gfxtype 246 -> <n>, build 308 -> <n>` deletes this page. One SubagentHandback.
