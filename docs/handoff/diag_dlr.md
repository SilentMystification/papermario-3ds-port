# diag_dlr: delta replay rows D1 to D4 (plan_dl_replay.md)

## State
- Branch worktree-agent-a39b22232eb3b937c. dlc-compiler-c1c2 merged (be296ab71 is an ancestor of HEAD).
- Ignored build inputs copied from the main folder (ver/us/build, assets/us, ver/us/asm/data, ver/current, 3ds/include/pm_build.h, 3ds/ld/map_restore.ld, build.ninja).
- Lane dlr. Azahar starts: PM_ROW=dlr1, 1 of 8 used.
- Run: D2 to D4 not started. D1 gate failed (stop rule in plan section 4).

## D1 result: gate FAILED, stop
- Code: pdl_op_rw read set fix (LOADBLOCK, LOADTILE, LOADTLUT, SETTILESIZE); gbi_fld table and gbi_pack (pm_dlc.inc.c); pdl_shadow (4 variants per address, 32-bit hash of packed read bytes); pdlsh report line in pdl_report; one call in the G_DL case (pm_gbi.c, before pdl_try).
- gbi_same not added: no D1 user. D5 adds it.
- gbi_fld placed in pm_dlc.inc.c, not pm_gbi.c: the GBIG_* bits are defined there.
- Build: lane dlr, build OK, no new warning in changed lines (pre-existing dlc_end const warnings in pm_gbi.c).

Numbers, Azahar yard (preset=yard phase fixedseed shotmode=4:80, 90 s, run 1 of 8, log build3ds/lane-dlr/last_log.txt):
- pdlsh calls per 300 frames: 0 in every window from f599 on. f299 window: 0 calls.
- pdl cls (pdl_try calls per dls_class) from f599 on: 0 for all classes. f299 window: class 2 1, class 7 37.
- Walk f300 to f599 (300 frames): bg 2.37 ms, main 9.53 ms, walk 11.91 ms. Interpreted ops 3576 per frame.
- dlc hit 68 per frame, ops 1120 per frame, G_DL walked 239 per frame.
- Tex closes 240.7 per frame, of which 166.0 empty. gbisplit: sync 729 us, gv 194 us, tex 4810 us.
- pdl hit 0, comp 0, refuse 0 (steady state). pdlm 0.

Verdict [M]: no class 2 or 3 G_DL target reaches the G_DL case or pdl_try in the yard steady state. The shadow cannot reach 70 %. Gate needs calls >= 1 for a ratio. Gate result: FAIL (calls 0).

Inference [I]: the class 2 state ops (1553 per frame, diag_dl dl3 R1) run from lists entered without a G_DL op from the walked lists, or from G_DL targets that pdl_refuse_ops already refuses. Pdl_cls counts before the refuse check, so a refused list would still show in cls. The cls count is 0, so the first cause is likelier.

Not run for D1: picture (shots needs the shots switch, a new start), statehash toad and battle, pause menu, file select. D1 changes no draw path, so these were not run after the gate failed.

## Next step (not run)
Stop per plan. The next diagnosis names where class 2 lists enter: a read-only agent counts the caller of each dls_class 2 list (pm_tex.c, pm_model, pm_nusys) for the yard steady state.

## M1 result (lane dlr2, PM_ROW=dlr2, 6 of 8 starts). Gate FAILED, stop.
Code: pdl_shadow removed. dlr_count (ungated by pdl state, gated by dlc_on && pm_ph_on) at pm_gbi.c G_DL case and pdl_tpl_gdl. Report line `dlr f<n>` in pdl_report. Numbers: 3ds/logs/perf/status_M1.txt.
- G_DL calls per frame (f300-599): class 1 68.0, class 2 65.0, class 3 73.0, class 7 33.0.
- Census ops per frame: class 1 1120, class 2 1109, class 3 365, class 7 1803.
- Refused ops: 0. nend 0. Class 3 lists read nothing (r0 73.0 per frame).
- Class 2: hits 8.0 per frame (same frame), hitx 0 (cross frame), miss 57.0 per frame. Hit ops 501 per frame. Bytes 456 per class 2 call.
- Gate: G1 hitops 501 per frame, need 1000: FAIL. G2 hit/acc 58.7 percent, need 70: FAIL. G3 456 bytes per call, need 256: FAIL.
- Inference [I]: hitx 0 because the class 2 entry is cleared when pm_pdl_heap_gen changes (pm_tex.c:762, TextureHeap writes). The gen changes often, so no entry survives a frame. Not measured.
- Walk with census on: bg 2.38 ms, main 11.43 ms, walk 13.81 ms, interp ops 3576 per frame. Without census (earlier D1 run): walk 11.91 ms. The main rise is census cost [I].
- Picture (shot 0040, yard): Mario, grass, shadow present. No pixel baseline file in repo. Statehash toad and battle: equal to bases at f100, f200, f300. Pause status text and file select text: readable.
- Plan section 7: M1 gate failed. E1 to E4, D5 and D6 not run.
