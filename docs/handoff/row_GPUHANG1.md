# Row GPUHANG1: GPU hang on the O3DS XL with fps60 (build 4576): citro3d re-sends a zeroed texture on unit 1/2 after a forced context

Model: Sonnet (user decides).
- Why not Haiku: the fix touches every place that zeroes or deletes a C3D_Tex that citro3d may still hold, in pm_tex.c and
  pm_gpu.c. The coder must find each site (list below) and keep the native texture shadow consistent.
- Why not Opus: the cause is named and the fix is one mechanism (a permanent dummy texture plus a unit shadow).
Priority: above every other row.

## User facts (fact, CLAUDE.md 1)
XL, build 4576 (git 67b270c94: INTERPB1-3, FPS60SPD, six native effects), push `prof warp=mac_01:0 pos=0:0:200 fps60`: the game
hangs. Log (build3ds/hw_o3ds_log.txt):
- `watchdog: no progress for 10 s, game thread last phase 7 (gpu end ...), steps 87`;
- `gx n 4 cur 4 last 1 e1 id 01 30189c00 00009490 ... e2 id 01 30193090 00004d40 ... e3 id 03 1f000000 3008ca00 019000f0 stereo 0`;
- `GPU stuck`.
The entries from lastEntry to curEntry are in flight (pm_gpu.c:2489-2497). So e1 is the stuck one: the first command list of a
walked mono frame, at the command buffer start (0x30189c00 = cb, the same base as INTERPB3's kcout lines), followed by a second
list and the frame-end display transfer. It is not a key-show frame: those lists live in kc_buf (pm_gpu.c:2925). Builds with
fps60 off ran (4492). Azahar shows nothing: it runs every GX command at submit and always raises the interrupt.

## Cause (oracle 3 code read, ranked)
1. (Named, first.) The forced context resend at every fps60 frame begin re-sends a zeroed texture held by citro3d on unit 1 or 2.
   a. INTERPB3 try 2 moved sr_force_all to sr_frame_begin for every fps60 mono frame (pm_gpu.c:2327: `if (!stereo_frame) { if
      (pm_interp_on > 0) sr_force_all(); return; }`). Before that, only stereo frames forced.
   b. sr_force_all sets the citro3d dirty flags 0x3800000 (texture units 0-2) and 0xFC000000 (pm_gpu.c:2317). The first
      C3Di_UpdateContext of the frame then rebuilds the unit enable word from the textures citro3d holds and sends each unit's
      block from its C3D_Tex (pm_gpu.c:1221 comment). That update writes into the frame's first list, which is e1.
   c. A texture-cache slot drop zeroes its C3D_Tex in place (`memset(&s->tex, 0, sizeof s->tex)`, pm_tex.c:803; also :265,
      :1518, :1614). It unbinds only unit 0 (`C3D_TexBind(0, NULL)`, :804; the comment there says unit 1 cannot be unbound with
      NULL). Unit 1 (the aux texture, C3D_TexBind(1, tp) at :2106, tp = &slots[aux].tex through tex_for_draw) keeps pointing at
      the zeroed struct.
   d. Result: the forced update sends unit 1 with data address 0, size 0 x 0 and format 0, and enables the unit. On hardware the
      PICA texture unit then reads physical address 0 or a 0-size texture and does not finish the list. That is the stuck e1.
   e. The map load at mac_01 (seamfix, texture misses) drops slots around the frames before 87.
   f. The same path explains the open STEREOFX P5 hang (row_STEREOFX.md): the 3D slider makes stereo frames, and sr_force_all ran
      for every stereo frame.
2. (Possible, second.) Other C3D_Tex structs citro3d may hold after their memory changed: offt[].tex (pm_tex.c:140, the render
   target textures of gs.offs, not freed per frame: low risk) and the local `tmp` of pm_tex.c:1530 (C3D_TexDelete: check that it
   is never bound). Check both in this row.
3. (Ruled out by read.)
   - kc_buf lists and key shows: the stuck list is not in kc_buf.
   - noise_next and the FPS60SPD pacing: values and waits only, no GPU words.
   - twin mallocs: a NULL is handled (pm_gpu.c:184-186), and the 674 KB margin is not a GPU fault.
   - load_mtx at frame 23: skipped by LOADMTX, a CPU guard.

## Fix (exact mechanism; the coder chooses the code inside it)
1. A permanent dummy texture: `static C3D_Tex pm_tex_dummy;`. Make it in pm_tex init with C3D_TexInit 8 x 8 RGBA8, filled with
   0, flushed, never freed.
2. A unit shadow `const C3D_Tex* c3d_unit[3]`, written at every C3D_TexBind call site: pm_tex.c:804, :946, :1998, :2021, :2088,
   :2106, :2113, and pm_gpu.c:809 dc_bind (grep `C3D_TexBind(` in 3ds/src; every hit goes through one wrapper
   `pm_c3d_texbind(unit, tex)`). The wrapper turns a NULL for units 1 and 2 into &pm_tex_dummy: C3D_TexBind(1, NULL) reads
   tex->fmt and faults (pm_tex.c:804 comment), and :2113 does exactly that today, so route it through the wrapper as well.
3. At every site that zeroes or deletes a C3D_Tex (pm_tex.c:265, :803, :1518, :1530, :1614): before the memset or delete, rebind
   any unit whose shadow points at that struct. Use NULL for unit 0, as :804 does, and &pm_tex_dummy for units 1 and 2. Reset
   the native shadow as a dirty bind does (nd_lt1 = NULL, au_last = NULL, tb_last = NULL as needed).
4. Debug census (cold): a counter of rebinds done by step 3 per unit, printed with the existing `mem` or prof lines every 300
   frames as `texunfix u0 <n> u1 <n> u2 <n>`. It shows on the console that the path happens.
5. No change to sr_force_all and no change to the fps60 design.

## Files and places
3ds/src/pm_tex.c (the zeroing sites, the bind sites, init), 3ds/src/pm_gpu.c (dc_bind :809 only), 3ds/include/pm_tex.h (the
wrapper declaration). No running row edits these places.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- `bench.py base spot_mac01_fps60_det`, `bench.py base spot_mac01_det`; one list
  `bench.py run spot_mac01_fps60_det,spot_mac01_det --row GPUHANG1 --why "texture unit rebind before slot zero"` (2 starts).
  Every shot byte-equal. The native draws write their own unit blocks, so the picture does not change.
- `texunfix` line on spot_mac01_fps60_det at f300: u1 or u2 > 0 confirms the path in this scene (hand back the line; 0 is also a
  fact to hand back).
- Console (the orchestrator, one push, user check): XL `prof warp=mac_01:0 pos=0:0:200 fps60` runs past 2 minutes. Then the user
  moves the 3D slider in kgr_01 (the STEREOFX scene).
- Budget 4 Azahar starts.

## Stop
A byte difference: stop, commit, hand back. Delete this row file in the completing commit.

## Until this row merges
Console pushes go without fps60, with the 3D slider at 0. In mono with fps60 off, sr_force_all never runs (pm_gpu.c:2327 needs
pm_interp_on > 0), and the K-copy code is off (kc_rec = pm_interp_on > 0, :2791). The stereo path keeps the old STEREOFX risk.
