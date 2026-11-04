# Texture cap toggle: 3072 KB and 1536 KB (TCT)

Scope: read-only trace of 3ds/src/pm_tex.c, pm_gpu.c, pm_gbi.c, pm_main.c, pm_mem.c (lane native, worktree papermario-3ds-port-main). No Azahar start in this run. The test in section 6 is proposed, not run.

## 1. VRAM layout (PROVEN from code)

VRAM is 6144 KB in two 3072 KB banks: bank A at 0x1F000000, bank B at 0x1F300000 (pm_mem.c:176-177, `pm_vram_bank_of` pm_mem.c:180). Users at boot and at runtime:

| User | Size | Bank | File:line |
|---|---|---|---|
| Texture cache halves | 2 x 1536 KB (one per bank) | A and B | pm_tex.c:22, 467-481 |
| Colour and depth targets | 240 x 400 x 4 each | A or B | pm_gpu.c:1303-1309 |
| Small arena (64 x 64 RGBA8) | 12 x 16 KB, 6 per bank, never freed | A and B | pm_gpu.c:341-350, 1310 |
| Copy block (copy_blk, also grab_tex) | 512 KB (BLK_FULL) | A first, then B | pm_gpu.c:353-378, 1792-1801 |
| Full off-screen images (512 x 256, OFF_N = 4) | 512 KB each | A first, then B | pm_gpu.c:389-400, 1716-1717 |

`bank_block` tries bank A, then bank B (pm_gpu.c:357-361). Only when both fail does it call `pm_tex_shrink` (pm_gpu.c:360).

Naming: "bank A" and "bank B" are the two VRAM banks. "Bank B half" is the 1536 KB cache half that sits in bank B (pm_tex.c:462, `bhalf`).

## 2. What drains bank B (PROVEN)

Drain (`pm_tex_shrink`, pm_tex.c:870-877) runs only when a 512 KB request (copy block or full image) fails in both banks. Its callers:

- Copy block: `copy_get` (pm_gpu.c:370-380), called from `grab_tex_init` (pm_gpu.c:1792-1793). `grab_tex_init` is used by the T2 window grab (`grab_cur`, pm_gbi.c:275-281, water splash, crystal ball, bulb glow) and by the T3 previous-frame grab (`pm_gpu_grab_prev`, pm_gpu.c:1829; caller `cfb_capture_prev`, pm_main.c:232-250, reached from pm_gbi.c:2991).
- Full images: `full_tex_init` (pm_gpu.c:389-400), called from `off_begin` when a draw uses an off-screen slot with stride above 64 (pm_gpu.c:1694-1717; trigger `off_screen_check`, pm_gbi.c:253, 290).

Not checked by name: the battle transition. It is only a candidate user, because it draws into an off-screen slot. The pause fill (D03, pm_gbi.c:3405) uses the CPU path (`cfb_capture_prev_cpu_for`), so it takes no VRAM.

## 3. Cap changes (PROVEN, file:line)

- Boot: `tex_cap` = 3072 KB when both halves come up (pm_tex.c:479). It is lowered only by the `texcap=<KB>` debug switch (pm_tex.c:790).
- Drop: `pm_tex_shrink` sets `bdrain = 1` and `tex_cap -= 1536 KB` (pm_tex.c:871-873). Log: "texcache shrink B: half draining, cap 1536KB" (pm_tex.c:876).
- Free: `bhalf_release` (pm_tex.c:878-891) runs at the start of a frame (pm_tex.c:928). It frees bank B only when no B slot is live and all 1536 KB of B are back in the extent table. Log: "texcache B half freed" (pm_tex.c:890). The cap stays at 1536 KB.
- Regrow: `bhalf_regrow` (pm_tex.c:892-901) runs at the start of a frame when `bhalf` is NULL, `bdrain` is 0, and `tc_frame - b_idle >= 120` (pm_tex.c:929). `b_idle` is set by `pm_tex_full_touch` each frame that a full image or the copy block is held (pm_gpu.c:417-418, called from `buffers_idle`, pm_gpu.c:1967). The cap returns to 3072 KB (pm_tex.c:899). Log: "texcache regrow B" (pm_tex.c:900). A failed regrow sets `b_idle` again, so the next try waits another 120 frames (pm_tex.c:894).
- Timing of one lower-and-raise cycle: full images are freed after 60 idle frames (`IDLE_FREE`, pm_gpu.c:352, 415). The last touch comes one frame before the free. The regrow then waits 120 more frames. So the cap is 1536 KB for about 180 frames after the last full-image or copy-block use. At 26 to 28 fps (console figure in project notes) that is about 6.5 to 7 s.

The HUD shows the live cap (`tex used/cap`, pm_mem.c:222, value from `pm_tex_budget`, pm_tex.c:796). So the flip the user sees is the `tex_cap` value.

## 4. What happens to cache entries in bank B (PROVEN)

- Drain marks every live B slot stale (pm_tex.c:874-875, `stale_mark` pm_tex.c:60). They are not evicted at once. They stay live and readable for the rest of the current frame.
- The next frame start drops the stale slots (`drop`, pm_tex.c:634-660; loop pm_tex.c:~906-925). `drop` returns the extent to `slab_free`. While `bdrain` is set, `slab_alloc` does not hand out B extents (pm_tex.c:514), so B stays empty.
- Entries that were in B are not moved to A. A later use of the same texture is a miss. It is loaded again into A (if A has room) or evicts something else. Ghost records are kept (`gh_put`, pm_tex.c:739).
- Eviction in the load loop uses the new, lower cap. The check is `used + reserve <= tex_cap` (pm_tex.c:1177). `used` still counts stale B bytes until the frame start. So a load in the drain frame can evict A entries if live A bytes are above 1536 KB (eviction skips only the current epoch, pm_tex.c:734-738). Measured live bytes in one light map were 658 KB (inv_texcache_evict.md), so this did not occur there. It is INFERRED to occur on a heavy map.

Garbled-picture risk after a drain (INFERRED, mechanism PROVEN):
- No texture corruption was found in the drop path. The GPU finishes the previous frame before `C3D_FrameBegin` (SYNCDRAW), and the stale drop runs after that.
- The visible risk is an off-screen pass that fails in the drain frame. `full_tex_init` returns 0, `off_fail` is set, and the pass draws on the screen for that frame (pm_gpu.c:1717-1719). `off_fail` clears at the next `pm_gpu_begin`. This matches the earlier note in agent_grab3.md ("stretched or garbled pictures"). It is one frame per drain.
- A copy-block failure makes `grab_tex_init` return 0. The grab then reads on the CPU (pm_gpu.c:1857 area, pm_main.c:242).

## 5. Toggle count and cost (INFERRED from code, counts need a log)

- Count per session: `grep -E "texcache (shrink B|regrow B|B half freed)" <log>` gives the cycles. Each drain is one "shrink" line, one "freed" line and one "regrow" line (the regrow line may be missing if VRAM is short).
- Frequency: a drain needs both banks short of 512 KB. Free space outside the cache halves is about 1000 KB per bank at boot (6144 - 3072 cache - 1536 targets - 192 arena = 1344 KB total for runtime; agent_grab3.md:23). Two or more held full images, or a held copy block, plus the cache, fill that space. So drains follow the use of the full-image and grab paths (splash, crystal ball, bulb, T3 grab, off-screen passes with stride above 64). A count per scene needs a log, not code.
- Cost of one drain cycle: bytes evicted = live B bytes at the drain (up to 1536 KB). Reload cost on demand = bytes re-missed x ns per byte. Measured miss-path cost: 74 ns/B (console, plan_tex_l2.md:145-146), 62 ns/B (Azahar, plan_tex_l2.md:147). Full B half, worst case: 1,572,864 B x 74 ns = about 116 ms on console; x 62 ns = about 97 ms in Azahar. Real cost is lower, because only textures used again are reloaded. The miss path is spread over later frames.

## 6. Does a fixed layout remove the toggle (INFERRED)

Yes, if the cap stays constant, the toggle goes away. Budget check, from agent_grab3.md:23 numbers:
- Total 6144 KB minus targets 1536 KB minus small arena 192 KB = 4416 KB for cache plus runtime.
- Runtime demand = copy block 512 KB + 4 full images 2048 KB = 2560 KB.
- Cache 3072 KB + demand 2560 KB = 5632 KB, which does not fit in 4416 KB.

So a fixed layout that reserves all runtime users at boot forces one of two choices:
- Option 1: reserve the 2560 KB pool at boot and set the cache cap to about 1856 KB (round down to 1792 KB). The cap is then constant, with no toggle. It is smaller than 3072 KB all the time, so heavy maps miss more.
- Option 2: keep the 3072 KB cache and cut runtime demand to 1344 KB or less. For example, a shared pool of 2 blocks (1024 KB), where a third block draws on the screen (visual loss in that pass).

Neither option keeps 3072 KB with the current demand.

## 7. Fix proposal

1. Pick option 1 or option 2 in section 6. This is a user decision (cache size against visual loss).
2. Implement the chosen layout at boot in `slab_init` (pm_tex.c:467) and `small_arena_init` (pm_gpu.c:341). Reserve the pool. Remove `bhalf_release` and `bhalf_regrow`, and the `bdrain` checks (pm_tex.c:514, 871-929). The toggle code is then dead. Delete it in the same row (no feature toggles, project rule 7).
3. Keep the `texcap` debug switch and the log lines.

## 8. Test (proposed, not run)

Goal: the cap value per frame in a battle start and in a pause open.

- Needed code: `tc_report` (pm_tex.c:826-850) prints no cap. Add `tex_cap` to the `texstat f<N>` line (one debug field, pm_tex.c:850). The HUD (pm_mem.c:222) does not reach a log.
- Justification line first: 3ds/logs/perf/status_TCT.txt, with the quantity (tex_cap per frame), the path (pm_tex_shrink, bhalf_regrow), and why no other run answers it (the existing logs show only the events, not the cap per frame).
- Run: `PM_ROW=TCT powershell -File 3ds/tools/azt.ps1 <scenario> -Row TCT`, one Azahar start, cap 25 s. Use an existing battle scenario from docs/handoff/test_catalog.md, and pause_spin (BTLSTAR) for the pause open. Add a full-image or grab trigger (for example `fxtest=89` for the splash, see test_catalog.md line 64) to force the drain. Use `vramfill=N` (pm_tex.c:484-497) to make bank B short at boot if the trigger is weak.
- Pass: the picture shows the subject (battle start, pause menu). The log shows `texstat f<N> ... cap` at 3072 KB before the trigger, then 1536 KB after "texcache shrink B", then 3072 KB after "regrow B" (or no regrow in the window). A fail is a cap change with no matching shrink or regrow line.

Files changed: docs/handoff/inv_texcap_toggle.md only.
