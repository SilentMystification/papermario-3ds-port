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
- SLOT1: reset once (ResetBudget SLOT1, user line). Starts used since the reset: 10 (runs 9 to 12 and the earlier 8 before the reset). Measurement runs 1 to 8 before the reset.
- TEXD1: reset once. Starts used since the reset: 3 (run 1 no line, run 2 frames 38 and 39 only, run 3 frames 200 to 203).
- AZLOCK1: no Azahar run.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py` (use /c/Python314/python; plain python is the Store alias), then `PM_LANE=native PM_ROW=<row> sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c from a copy.
- Test: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build native -Try`
- Logs: the game log is build3ds/lane-native/last_log.txt (shared with other runs; copy it right after the run).
- Scenarios added: kzn19_slotpeak, battle_slotpeak, pause_slotpeak, chain_slotpeak (SLOT1), hos01_texword (TEXD1). Catalog rows in docs/handoff/test_catalog.md.
- Not committed and not touched: 3ds/src/map_restore_table.c, 3ds/logs/perf/pushed_builds.txt. Another agent's DIAG edit in src/evt/script_list.c was in the tree during this session (reverted by its owner before the commits).
