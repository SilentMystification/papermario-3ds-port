# Row SPLASHPREV2: water_splash refracts the buffer before the current one (3 frame buffers in the world)

(Model: Haiku. Base: main. Edits src/effects/water_splash.c only (the refrBuf line :210). Disjoint from every running row. Reads this
file and CLAUDE.md.)

## User facts (console fxtest, O3DS)
water_splash +12.3 ms per frame above the base (53 draws, 65 texture calls).

## Cause (oracle code read)
- The previous-frame GPU copy exists (PT12): a set_timg of a frame buffer that is not the current one calls pm_cfb_capture_prev
  (nd_tex_set.c:47 -> pm_main.c:347), which asks pm_gpu_grab_prev for the slot of the buffer BEFORE the walker's current one
  (`nuGfxCfb[(i + nuGfxCfbNum - 1) % nuGfxCfbNum]`, pm_main.c:356-358; the copy is queued at the next frame begin, pm_gpu.c:2526).
  The CPU copy (pm_gpu_read_frame, pm_main.c:380) runs only when the GPU copy is not ready.
- SPLASHPREV picked the other buffer of two (water_splash.c:210: `nuGfxCfb[0] == nuGfxCfb_ptr ? nuGfxCfb[1] : nuGfxCfb[0]`, its
  comment says nuGfxCfbNum 2). The world runs 3 buffers (nuGfxSetCfb(FrameBuf, NU_GFX_FRAMEBUFFER_NUM), nugfxinit.c:65; a battle 2,
  state_battle.c:58). With 3 buffers, 2 frames of 3 the splash reads a buffer that has no slot (the slot is made for the previous one),
  so each drop's LoadMultiTile loads from that buffer's RAM (stale picture, a texture load per drop).
- water_block reads no frame buffer (no cfb in water_block.c): its cost is draws and texture calls, not refraction (see row_ORACLE_NEXT).

## Start
`sh 3ds/tools/wt_new.sh SPLASHPREV2`. PM_ROW=SPLASHPREV2 PM_LANE=SPLASHPREV2 on every build and bench call.

## Exact edit (src/effects/water_splash.c:210, the TARGET_3DS branch)
Replace `u16* refrBuf = nuGfxCfb[0] == nuGfxCfb_ptr ? nuGfxCfb[1] : nuGfxCfb[0]; /* SPLASHPREV ... */` by
`u16* refrBuf = nuGfxCfb[0]; { u32 k_; for (k_ = 0; k_ < nuGfxCfbNum; k_++) if (nuGfxCfb[k_] == nuGfxCfb_ptr) { refrBuf = nuGfxCfb[(k_ + nuGfxCfbNum - 1) % nuGfxCfbNum]; break; } }`
`/* SPLASHPREV2: the buffer before the current one (2 or 3 buffers), the one pm_cfb_capture_prev copies on the GPU */`
(nuGfxCfbNum: extern u32 in the nusys headers; include what the file needs.)

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; `bench.py run nd_host --row SPLASHPREV2 --why "water_splash source buffer"`: PASS.
- `bench.py base spot_mac01_det`; one list `bench.py run spot_mac01_det,ndref_splash_pic,fxcen_c5 --row SPLASHPREV2 --why "splash reads
  the copied buffer"`; `bench.py cmp spot_mac01_det --row SPLASHPREV2`: 6/6 byte-equal; `bench.py cmp ndref_splash_pic --row SPLASHPREV2`
  (base first: `bench.py base ndref_splash_pic`): write the per-shot diff counts (a change is expected in 2 of 3 frames).
- `fxcen_report.py` on the fxcen_c5 LOG: copy: write the water_splash row (tex, texmiss) in the commit. The console ms is the user's check.

## Step 2 (after bfe0ac756: spot_mac01_det 6/6 byte-equal, fxcen_c5 ran; ndref_splash_pic has no shots and varying counts: dropped)
No more Azahar starts (budget used). 1. `python 3ds/tools/fxcen_report.py <the fxcen_c5 LOG: copy> --label splashprev2` (no start).
2. Gate: the water_splash row (index 89) has texmiss per frame not above the FXCEN2 Azahar table (build3ds/out/fxcen_azahar.txt) and
tex not above it; write both rows in the commit. 3. Commit, delete this row file; the console ms is the user's check.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the water_splash row.
