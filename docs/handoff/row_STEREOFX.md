# Row STEREOFX: 3D slider hang and crashes on the O3DS XL (code read only, no repro: CLAUDE.md 1)

(Model: none yet. Status: every path that the code read can name is fixed or ruled out; the open path needs the console dump.)
## User facts
1. XL, 3D slider moved up in a scene (omo dark room with Watt; kgr_01 after mac_05): the game hangs or crashes at the slider move.
2. XL, talking to Merlee with 3D on: crash right before the price screen.
## Paths (3ds/src/pm_gpu.c at 141c5e371) and status
- P1 replay buffers from malloc (NULL on the first stereo frame): FIXED. sr_ent and sr_fb are static arrays now (:1965-1966).
- P2 menu present (pm_gpu_menu_present_w :1621-1632): sets stereo_frame with the u_eye test (:1625), clears sr_on and draws both
  targets with no replay. No crash or hang path (a frozen menu frame is presented as-is).
- P3 a replayed GX copy whose destination is the left target (sr_replay): a wrong picture at most, no hang.
- P4 the toggle itself: gfxSet3D only sets the top mode (libctru gfx.c); the right framebuffer exists from gfxInit; fills on targetR
  are mapped by sr_map; the queue stays under 32 entries (pm_gx_room); every list is padded and finalized (GPUCMD_Split): ruled out.
- P5 OPEN, the hang: libctru gxCmdQueueWait(-1) (pm_gx_drain :2145-2154, and every C3D_FrameBegin) waits for the PSC0/PPF/P3D/DMA
  interrupt of each submitted entry; a hang means one submitted entry whose interrupt never comes. On hardware that is a PICA lockup in
  a replayed command list, most likely from GPU state at the start of the right pass that sr_force_all (:1986-1995) does not rewrite.
  Azahar runs every GX command at submit and always raises the interrupt, so it cannot show this path.
- P6 OPEN, the Merlee crash on Azahar (AZTSTEREO, oracle batch 4): a stray 32-bit store of a heap pointer into RenderTaskListIdx
  during render_effects_scene. The writer is not named by code read.
## What names the fix (user action, console only)
ESCAPE (merged) dumps the stuck GX queue at START+SELECT: "escape: START+SELECT ... gx n <n> cur <c> last <l> e<i> id .. <words>"
in crash.txt. Push main with logging on (not a prof push), move the slider in kgr_01 or the omo dark room, wait about 12 s, hold
START+SELECT 1 s, pull crash.txt and log.txt. The in-flight entry id and its list address name the register or list for P5.
For P6: the next console crash shows the crash page with PC and LR (CRASHALL); pull crash.txt.
## Next
The oracle reads crash.txt and writes the fix row (one path, exact edit). No Azahar run in this row; budget not used.
