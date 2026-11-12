# Row GPUHANG1: XL GPU hang with fps60: never leave a zeroed C3D_Tex bound on a citro3d texture unit (FIRST PRIORITY)

(Model: Haiku (user choice). Exact steps; no design choice. Base: main fc3f6ad53 or later.)

## User fact and cause (oracle 3 code read)
XL build 4576, push `prof warp=mac_01:0 pos=0:0:200 fps60`: the game hangs. `GPU stuck` at f87; the stuck entry is e1, the first
command list of a mono frame (0x30189c00).
- Every fps60 frame runs sr_force_all (pm_gpu.c:2327). It marks citro3d texture units 0-2 dirty (:2317), so the next
  C3Di_UpdateContext re-sends every unit from the C3D_Tex that citro3d holds.
- A texture slot drop zeroes its C3D_Tex in place (pm_tex.c:803) and unbinds only unit 0 (:804). Unit 1 keeps the zeroed struct
  (bound at :2106).
- The re-send then gives unit 1 data address 0 and size 0, and the PICA hangs on hardware. The 3D slider hang (STEREOFX) is the
  same path in stereo frames.
- Off-screen textures (offt[].tex, gs.offs[].tex, pm_gpu.c:2096-2105) are made once and never zeroed: no change for them. The
  local `tmp` at pm_tex.c:1530 is never bound: no change.

## Start
`sh 3ds/tools/wt_new.sh GPUHANG1`; PM_ROW=GPUHANG1 PM_LANE=GPUHANG1.

## Touch only
3ds/src/pm_tex.c (the places named below), 3ds/src/pm_gpu.c (dc_bind :809 and one prototype above it).

## Exact edits
1. No header change. pm_tex.h does not include citro3d.h, and C3D_Tex is an anonymous typedef that cannot be forward-declared. In
   pm_gpu.c, put one prototype `void pm_c3d_texbind(int unit, C3D_Tex* t);` right above dc_bind (:809). pm_tex_unit_drop is used
   only inside pm_tex.c. pm_tex_bind_reset is declared in pm_tex.h:33, and pm_fl_dc in pm_port.h:188.
2. pm_tex.c, right after the includes (after line 10):
   ```
   static C3D_Tex tex_dummy; static int tex_dummy_ok; /* GPUHANG1: 8 x 8 zero texture for units 1 and 2 (C3D_TexBind(1, NULL) faults) */
   static const C3D_Tex* c3d_unit[3]; /* GPUHANG1: the texture citro3d holds per unit */
   static unsigned texunfix_n;
   void pm_c3d_texbind(int unit, C3D_Tex* t) { if (unit >= 0 && unit < 3) c3d_unit[unit] = t; C3D_TexBind(unit, t); }
   void pm_tex_unit_drop(const C3D_Tex* t) { /* before a C3D_Tex is zeroed or deleted: no citro3d unit may keep it */
       for (int u = 0; u < 3; u++) {
           if (!t || c3d_unit[u] != t) continue;
           if (u == 0) pm_c3d_texbind(0, NULL);
           else if (tex_dummy_ok) pm_c3d_texbind(u, &tex_dummy);
           pm_tex_bind_reset(); /* the native unit shadows (tb_last, au_last, nd_lt, nd_lt1): the next draw writes its unit block */
           if (texunfix_n++ < 32u) pm_log_d("texunfix f%u unit %d", pm_frame_count(), u);
       }
   }
   ```
   If pm_log_d, pm_frame_count or pm_tex_bind_reset have no declaration at this point, add `extern` prototypes above the block
   (`void pm_log_d(const char*, ...); unsigned pm_frame_count(void); void pm_tex_bind_reset(void);`).
3. pm_tex_init (pm_tex.c:1006), at its end before `tb_init();`:
   `if (C3D_TexInit(&tex_dummy, 8, 8, GPU_RGBA8)) { memset(tex_dummy.data, 0, tex_dummy.size); pm_fl_dc(tex_dummy.data,
   tex_dummy.size); C3D_TexSetFilter(&tex_dummy, GPU_NEAREST, GPU_NEAREST); tex_dummy_ok = 1; }` (if pm_fl_dc is declared later in
   the file, use the same call it already uses at :268, adding its prototype above pm_tex_init if needed).
4. Every C3D_TexBind becomes pm_c3d_texbind, with the same arguments: pm_tex.c:804, :946, :1998, :2021, :2088, :2106, :2113, and
   pm_gpu.c:809 (dc_bind). Do not change the arguments. After the edit, `grep -n "C3D_TexBind(" 3ds/src` shows only the one
   inside pm_c3d_texbind.
5. Before every place that zeroes a C3D_Tex of a slot, add `pm_tex_unit_drop(&s->tex);` on the line before it:
   - :803, the memset in the slot-drop block. Keep the line at :804: it is now pm_c3d_texbind(0, NULL), and the drop rebinds
     units 1 and 2.
   - :265 (cell_slot_init, before its first memset);
   - :1518 (before `memset(&s->tex, 0, sizeof s->tex);`);
   - :1614 (before the `s->tex_vram_ptr = NULL; ...` line).
   Leave :266 alone: it zeroes after a failed init, in the same function as :265.
6. Pre-flight before the build: `grep -n "memset(&s->tex" 3ds/src/pm_tex.c` lists exactly :265, :266, :803, :1518, :1614 (lines
   may move by your edits, the count must be 5). If there is another zeroing of a C3D_Tex, stop and hand back the line.

## Gate (CLAUDE.md 2-3)
- `PM_ROW=GPUHANG1 PM_LANE=GPUHANG1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row GPUHANG1 --why "GPUHANG1"`: PASS.
- `bench.py base spot_mac01_fps60_det`, `bench.py base spot_mac01_det`. Then one list:
  `bench.py run spot_mac01_fps60_det,spot_mac01_det --row GPUHANG1 --why "texture unit rebind before slot zero"` (2 starts). Then
  `bench.py cmp` both: every shot byte-equal.
- Hand back the count and the first 4 `texunfix` lines of the spot_mac01_fps60_det log (0 lines is a fact to hand back, not a
  failure).
- Budget 4 Azahar starts. The console check is the orchestrator's: one XL push with fps60 that runs past 2 minutes, then a 3D
  slider move in kgr_01.

## Stop
A byte difference, a build error, or a C3D_Tex zeroing site not in the list: stop, commit, hand back. Delete this row file in the
completing commit.
