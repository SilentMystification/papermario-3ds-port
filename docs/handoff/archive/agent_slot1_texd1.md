# agent_slot1_texd1: rows AZLOCK1, SLOT1, TEXD1 (branch native, worktree papermario-3ds-port-main)

## Commits
- 02e901772 AZLOCK1: run_azahar shot move and last_log copy in try/catch, inside the Azahar lock (Release-Lock at the last read). Tool file only. Parse OK. Not run in Azahar.
- 43e03abbb SLOT1: SLOTS 512 to 1024, free slot search reads live_bits (32 words), texstat slots line (live, peak, limit, under 1 KB, byte cap). Numbers in the commit message.
- TEXD1 debug commit (next): texword dump moved to pm_tex_bind (the path the draws use), texword=<hex>@<frame> frame gate, hos01_texword scenario, catalog row. Debug only. Not a fix.

## SLOT1 result (answer to the coordinator question)
- kzn_19 peak live slots, uncapped build (SLOTS 2048): 327 (f300), 458 (f600), 488 (f900), 494 (f1200), 497 (f1500). Still rising about 3 per 300 frames.
- Under 1 KB: 35 of 497 at f1500. Live bytes f1500: 1175 KB (2.4 KB average). Byte cap 1856 KB not reached.
- Battle (battle switch, kmr_03): peak 142 at f600. Pause open: peak 149 at f300. Chain mac_00, mac_01, kzn_19, mac_01: peak 239 at f1500 (kzn_19 stay was about 80 frames, not the full peak).
- Rule: 497 x 1.5 = 746, next power of two 1024. Set SLOTS 1024.
- RAM: Slot is 136 bytes. .bss.slots 0x22000 (139264 B) at 1024, 0x11000 at 512. +68 KiB. Live bitmap 128 B. 2048 would be 272 KiB.
- Final build at 1024: kzn_19 f600 459 live, 0 evictions (same as 512). mac_01 f600 104 live, 0 evictions.
- Byte-equal (pm_tex.c is on the draw list): yard top shot 0000_0_11 equal to HEAD (512). mac_01 shots 0000_1_2 and 0001_1_2 equal to HEAD. Bottom screen differs only in frame time and build suffix text.
- Open (not fixed): lookup fallback is still a linear scan of slot_hi (after the 256-entry lidx probe and the 12-entry recent list). Scan length doubles at 1024 (136 B records). Not measured on console. Candidate: LIDX_N 256 to 1024 (2 KB short array). Eviction scan (evict_oldest_done) is O(slot_hi) per eviction; 0 evictions in all measured runs.
- Byte cap binds before the slot limit at kzn_19 (about 790 slots at 2.4 KB average). The slot limit is not the binding limit at 1856 KB.
- Battle run with encwarp=120 (scenario battle_slotpeak, first version) stalled at frame 174 with crash symbols. Same path as ENC1. Not used.

## TEXD1 state (not fixed)
- The wall in this build is texture 0x0100169c (chunk #27, frame 200 of hos_01). Earlier note 0x0100066c was from an older build.
- Wall chunk #27 at frame 200 (drawlog): tex 1 timg 0100169c fmt 4 siz 0 64x64 tri 5, tw 00000000 00000000 00000005 00000005 (cms 0, cmt 0, masks 5, maskt 5: repeat).
- texword at frame 200 to 203 (run 3): wrap s 3 t 3 (GPU mirrored repeat), shr 0 0, slot 8, gpu 64x64 gn 4096 (LA4), hash 3470b4d1. Mismatch: bind uses wrap_s/wrap_t, which hold the wrap of the last pm_tex_set_wrap call.
- Root cause (code reading, to prove by test): pm_gbi.c ensure_tile(n): the cached path (tile_bound(n), line ~2578) returns without pm_tex_set_wrap, so the bind uses the wrap of the last load. Second cause: dual_bind (pm_gbi.c ~1218) calls pm_tex_set_wrap with tile 1 wrap for the aux load, which overwrites the unit 0 wrap state (wrap_s, wrap_t are shared).
- The texture itself is expanded with the mask (pm_tex.c expand, wrap_mod with mk_ms), so the content is built for repeat or mirror at load. A bind with the wrong wrap then samples a different period (N64 repeat with mask 5 is a 32-texel period, built into the 64 texel texture by tiling).
- Not proven yet: that the wrong wrap changes the picture on console. Azahar shows the same wrap state (same code path), so the garble may need a console check.

## Next steps (TEXD1)
1. Fix: store the wrap per tile at ensure_tile (both cached and loaded paths), and make pm_tex_bind use the wrap of the draw's tile (not the last load). Prefer: pm_tex_set_wrap skipped when raw cms/cmt/masks/maskt equal the last call (a sync helper), called in the cached path of ensure_tile and after the aux load in dual_bind (restore tile 0 wrap). Check the call order in pm_gbi.c before writing.
2. Gate: hos01_texword (frames 200 to 203) shows wrap s 2 t 2 (GPU repeat) for 0x0100169c. The picture of hos_01 changes at the wall only.
3. Byte-equal on yard (kmr_20) and mac_01 top screen against HEAD. A change there is a finding: report it, do not hide it.
4. Also check tik_03 coins (RGBA16 32x32, tex 0x00571b90, mirror masks 5 5). Mirror on a 32 texel image with mask 5 is correct only when the coordinates stay inside 0..31 (window 31 texels). Check the tik_03 texture address in this build first (drawlog f200).

## Budget and tests
- SLOT1: reset once (ResetBudget SLOT1, after run 8). Starts used in total: 12 (runs 1 to 8 before the reset, runs 9 to 12 after it: yard, yard baseline, mac_01, mac_01 baseline). Each start has its line in 3ds/logs/perf/status_SLOT1.txt.
- TEXD1: reset once. Starts used since the reset: 3 (run 1 no line, run 2 frames 38 and 39 only, run 3 frames 200 to 203).
- AZLOCK1: no Azahar run.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py` (use /c/Python314/python; plain python is the Store alias), then `PM_LANE=native PM_ROW=<row> sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c from a copy.
- Test: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build native -Try`
- Logs: the game log is build3ds/lane-native/last_log.txt (shared with other runs; copy it right after the run).
- Scenarios added: kzn19_slotpeak, battle_slotpeak, pause_slotpeak, chain_slotpeak (SLOT1), hos01_texword (TEXD1). Catalog rows in docs/handoff/test_catalog.md.
- Not committed and not touched: 3ds/src/map_restore_table.c, 3ds/logs/perf/pushed_builds.txt. Another agent's DIAG edit in src/evt/script_list.c was in the tree during this session (reverted by its owner before the commits).

## Session 2 (code agent, branch native): TEXD1 result, SLOT1 follow-up, VRAM3 not started

### TEXD1: stale-wrap hypothesis refuted (no wrap fix committed)
- Tried: ensure_tile sets the tile wrap on every draw (flush before a change), pm_tex_set_wrap skips equal fields. Reverted (no change in the bind wrap).
- Bind wrap at the texword line: wrap s 3 t 3 (mirror) for all 40 binds of 0x0100169c, frames 200 to 239 (run 8). Same value with and without the fix (runs 3 to 8).
- The ensure line before a bind shows the same tile wrap (mirror, e.g. tw 1 1 6 6). So the bind already matches the tile state of the draw. The stale-wrap cause is not the reason for this texture.
- Open: the drawlog tw (cms 0, cmt 0, masks 5) of the wall chunk is not the tile that samples 0x0100169c. The drawlog does not print in the scenario log now (no line with tw or timg). Next diagnosis: find which tile and which draw sample 0x0100169c at frame 200 (drawlog output, or a log line in the draw path with the tile number and the triangle count).
- Commits: ebc4ac407 (debug), catalog row updated. No code fix.
- tik_03 coin gate not run (budget). Byte-equal gates for TEXD1 not run (no change to keep).
- Budget TEXD1: 8 of 8 starts used (runs 1 to 8). Cap reached. Gates not run: tik_03 coin shot. Row state: INCOMPLETE for the coin gate, FAILED for the wall hypothesis.

### SLOT1 follow-up: done (commit 1413ef8c3)
- pm_tex.c: lookup walks the key bucket (lhead, lnext, lkey; 256 buckets). load_miss links, drop unlinks. The single-entry lidx probe and the linear scan over slot_hi are removed from load_impl.
- Byte-equal: yard top shot 0000_0_11 and mac_01 0000_1_2 and 0001_1_2 equal to the 1024 slot build (SLOT1 runs 13 and 14, status_SLOT1.txt).
- Not measured: the time of the lookup on console. The chain length at 497 live slots is about 2 per bucket (not counted in a run).
- Open: pm_tex_load_rgba8 (the !rgba path, pm_tex.c) still scans slot_hi (linear). Check if it is on a hot path before changing it.
- Budget SLOT1 since the reset: 6 of 8 used (runs 9 to 14, runs 9 to 12 before this session). Two left.

### VRAM3: not started
- Step 1 (trace when each of the 4 off-screen full images is live) is not run. Existing hint: the V05 log lines in pm_gpu.c:1681 to 1691 print when a fifth image is wanted (slots used again).
- Steps (a) RGBA5551, (b) fewer images, (c) shared depth: not started. Each needs its gates (boot line, yard and mac_01 shots, pause_spin, battle_start_fb frames 0 to 210).

## Session 3 (TEXD2, code agent, branch native)

### TEXD2 switches (code in pm_tex.c, default behaviour unchanged)
- wrapov=repeat|mirror|clamp|edge: forces the PICA wrap of every unit 0 and aux bind (clamp = CLAMP_TO_BORDER, edge = CLAMP_TO_EDGE).
- texnearest=1: nearest filter on unit 0 and aux.
- padov=off: the texture pad (pm_tex.c expand, pad loop) uses the plain edge texel, no mask follow. Other pad paths (tx1 build) are not changed.
- texreg (debug word): one line per slot instance: `texreg img <addr> slot <n> conv <fmt><siz> native <0|1> w h gpu WxH pad x y mask s t mirror s t shr wrap s t filter <f>`. Logged once per slot instance (sgen).
- Scenarios texreg_wall (hos_01, until scene frame 300) and texreg_coin (tik_03, until scene frame 300). Catalog rows added.
- Build passes. No Azahar line has been read yet. The texreg line never appeared (runs 1 to 6).

### Open: the wall is not bound on the bucket lookup build
- A/B: pm_tex.c from 124e1d468 (before LCH1), scenario hos01_texword: texword bind at frame 200 (wall, wrap 3 3).
- Bucket lookup build (1413ef8c3 and later, with TEXD2): no bind of 0x0100169c by scene frame 300 (texreg runs 1 to 3, 5 and 6, texword run 2).
- Temporary check (removed): after a chain miss, a linear scan of the slot table found no slot the chain missed (run 6, zero lchcheck lines). So the chain is not losing a matching slot at lookup.
- Not yet known: whether the scene path differs (the wall draw never requests the texture) or the lookup does. Next step: A/B again with the same scenario and the frame-300 end. Compare the texbind counts (texstat / texbind lines) of both builds. If the bucket build is at fault, revert 1413ef8c3 (pm_tex.c lookup) and keep the texreg work.
- Run 4 A/B used the pre-LCH pm_tex.c with the current scenario list (no TEXD2 switches).

### Budgets
- TEXD2: 6 of 8 starts used (texreg runs 1 to 3 and 5 and 6, A/B run 4).
- Not started in session 3: VRAM3 (step 1 trace), pm_tex_load_rgba8 linear scan fix (step 3).

### Session 3 update: revert and the regression check (supersedes the bucket-lookup suspicion above)
- Reverted the bucket lookup: commit ac6df01b5 (git revert of 1413ef8c3). The texword dump cap is back to 4 with it.
- Reverted HEAD (ac6df01b5, which still contains TEXD2 0f156f4c7): scenario texreg_wall (until scene frame 300): no texreg line, no bind of 0x0100169c. Scenario hos01_texword: no texword bind of 0x0100169c by frame 204.
- The A/B build (pm_tex.c from 124e1d468, no TEXD2, scenario hos01_texword): texword bind 1 at frame 200, wrap s 3 t 3 (run 4 earlier). This build differs from ac6df01b5 only in pm_tex.c.
- So the bucket lookup is not the proven cause. The prime suspect is the TEXD2 change to pm_tex.c (commit 0f156f4c7): the switch parser, the pad loop guard, the wrap and filter override (ff), the aux override, texreg_log. The defaults should not change behaviour, but the wall bind does change.
- Next step (fresh agent): `git diff 124e1d468 ac6df01b5 -- 3ds/src/pm_tex.c` and bisect the TEXD2 hunks (apply each to 124e1d468, run hos01_texword). Check first the bind shortcut in pm_tex_bind (tb_f, tb_ws) and td2_parse calls in the hot path.
- The bucket lookup stays an open row. Its test must show that the same set of textures binds on hos_01 (A/B on the same scenario) before it is merged again.
- Wall texreg values: NOT obtained. No texreg line appeared in any run (runs 1 to 3, 5 to 7), so the values for 0x0100169c and 0x00571b90 are still unknown.
- Budget TEXD2: 8 of 8 used. Reset needed for further starts. Coin gate (texreg_coin) not run.
- Not started: VRAM3 step 1 (trace of the four off-screen images), pm_tex_load_rgba8 linear scan fix (bucket lookup, after the test above).

## BIS1: the wall bind (session 4, branch native)

### Determinism (runs 1 to 4, scenario hos01_texword, cap 25 s)
- 124e1d468 (worktree papermario-3ds-port-bis, lane bis): runs 1 and 2 both bind 0x0100169c at texture frame 200, slot 8, wrap s 3 t 3, shr 0 0, hash 3470b4d1. Deterministic.
- HEAD 6089edad6 (lane native): runs 3 and 4 both show no bind by frame 204. Deterministic.
- The two builds differ only in 3ds/src/pm_tex.c (commit 0f156f4c7 TEXD2 hunks; 1413ef8c3 bucket lookup was reverted in ac6df01b5, so HEAD pm_tex.c is 124e1d468 plus the TEXD2 hunks).

### Bisect
- Run 5, variant V1 = 124e1d468 plus the TEXD2 hunks minus the pm_tex_bind hunk (expand pad guard, texreg_log function and aux override kept): bind at frame 200, wrap 3 3. The bind hunk alone removes the bind.
- The bind hunk (commit 0f156f4c7) is the first bad change. Its default-path semantics look the same on reading (td2 switches absent, ff equals tex_filt, pm_debug_value has no side effects). The cause is not found in the code. It may be a layout or timing effect of the extra code. Not proven.
- 1413ef8c3 (bucket lookup) was not built alone. It is not needed: HEAD pm_tex.c without the bucket code still fails.

### Fix
- pm_tex.c reverted to 124e1d468 (all TEXD2 switches removed from the draw paths). Log-only change: the texword line now prints `filter <tex_filt>` (tentative declaration of tex_filt above texword_dump).
- Scenarios texreg_wall and texreg_coin now use the texword probe (debug `texword=<addr>`, no frame, first 4 binds from frame 0). The texreg switch no longer exists.
- Run 6 (fixed HEAD, texreg_wall): `texword f38 img 0100169c bind 1: slot 8 64x64 fmt 4 siz 0 ... gpu 64x64 ... staged 1 wrap s 3 t 3 shr 0 0 filter 1`. The wall binds again. Log checks 4 of 4. Picture not read by the agent.

### Programmed values
- 0x0100169c (hos_01 wall, I4 64x64): wrap s 3, wrap t 3 (GPU_MIRRORED_REPEAT, shr 0 0), filter 1 (GPU_LINEAR), slot 64x64, GPU 64x64, pad 0 0, mask and mirror flags not in this line.
- 0x00571b90 (tik_03 coin): NOT obtained. Run 7 (fixed HEAD, texreg_coin, until scene frame 300): no texword line, so the image is not bound by pm_tex_bind in tik_03 entry 0 within 300 frames. Next step: find a position where the coins draw (the display list shows timg 0x00571b90, RGBA16 32x32, cms 1 cmt 1, masks 5 5, see trace_tex_geom.md), then run texreg_coin there.

### Budget
- BIS1: 7 starts used (runs 1 to 7). Budget cap 8.
- The bis worktree (papermario-3ds-port-bis, detached at 124e1d468) is a build copy. Remove it with git worktree remove when done.

### VRAM3
- Not started in this session (see the VRAM3 section above for the planned steps).
