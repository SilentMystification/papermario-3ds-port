# ws_ROMDR (Haiku): the player raster ROM read skips the walker drain
Designer: oracle NATIVE19 (plan_walk.md 1). Worktree build3ds/wt/ROMDR, branch ROMDR. PM_ROW=ROMDR PM_LANE=ROMDR. Start: main after STEPX merges.
WAIT: merge only after the STEPX console capture (that run is the before). Coding and the gate can run now.
Why Haiku: one rename with a flag parameter, 2 wrappers, one call site, given below. Indentation: as in the file.
FACT: nuPiReadRom (3ds/src/pm_os.c:490-541) drains the walker queue before every non-audio read (:492-495). spr_get_player_raster
(src/101b90_len_8f0.c:332-363) reads into a cache entry with lazyDeleteTime 0: that raster was last drawn 2 or more steps ago (:341, :356; the minus 1
per step is spr_render_init, src/sprite.c:911-912, src/main_loop.c:204). The walker has walked that frame and its key frame before this step (nd_wq.c:162-163).
The queued frame draws only entries with lazyDeleteTime above 0. So the walker never reads the target bytes: the drain is not needed for this read.
Objective: walking steps without a drain per raster change. Counter: console stepx (walk bucket ft, slow count, d), core2 drain dr.
## Edits (CRLF files: use the Edit tool)
1. pm_os.c:490. Before: `void nuPiReadRom(u32 rom_addr, void* buf, u32 size) {`
   After: `static void rom_read(u32 rom_addr, void* buf, u32 size, int drain) { /* ROMDR: drain 0 only for a target the walker cannot read now */`
2. pm_os.c:492. Before: `    if (!pm_on_audio()) { /* CORE2-3b: a ROM read writes RAM that the last walk may have read */`
   After: `    if (drain && !pm_on_audio()) { /* CORE2-3b: a ROM read writes RAM that the last walk may have read */`
   If STEPX added `pm_rom_lg_n++;` inside that block, move it above the `if` (it counts every non-audio read).
3. After the closing `}` of that function (:541) add:
   `void nuPiReadRom(u32 rom_addr, void* buf, u32 size) { rom_read(rom_addr, buf, size, 1); }`
   `void nuPiReadRomOwn(u32 rom_addr, void* buf, u32 size) { rom_read(rom_addr, buf, size, 0); } /* ROMDR: a free player raster cache entry (101b90_len_8f0.c) */`
4. src/101b90_len_8f0.c:361. Before: `    nuPiReadRom(SpriteDataHeader[0] + (playerRasterInfo & 0xFFFFF), cacheEntry->raster, (playerRasterInfo >> 0x10) & 0xFFF0);`
   After: `    { extern void nuPiReadRomOwn(u32 rom_addr, void* buf, u32 size); nuPiReadRomOwn(SpriteDataHeader[0] + (playerRasterInfo & 0xFFFFF), cacheEntry->raster, (playerRasterInfo >> 0x10) & 0xFFF0); } /* ROMDR */`
   Do not change the other nuPiReadRom calls (:296, :319, :323 run at a map load).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=ROMDR sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row ROMDR`: byte-equal.
   Then the same 3 commands for spot_mac01_drec: byte-equal. (Det runs are serial: a drain waits for nothing there, so the bytes must not change.)
Fix rights: compile errors, the declaration of rom_read above its first use. A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `ROMDR 1: gate. <host> <c0gate> <2 cmp lines>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
Risk (console only): a wrong player sprite frame for one step if a queued frame still drew the entry (the analysis above says no). Fallback: revert.
