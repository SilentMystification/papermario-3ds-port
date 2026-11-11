# Row MIM11CEN: which draw path drops the mim_11 floor (measurement, existing census, no code edit)

(Model: Haiku. Bug: user fact (console, after R13NOP): the floor of mim_11 (Forever Forest, outside Boo's Mansion) does not render.)

## Code read (oracle, main 6e171b39a)
- Fixed since the old rows: FFOREST far clamp (3ds/shaders/pm5.v.pica:68-73, pm4.v.pica:57-58) and FFOG1 fog at w <= 0 (pm5.v.pica:141).
  Neither applies to mim_11: its scripts set no fog and no camera far plane (src/world/area_mim/mim_11/main.c:241-255: EVS_Main sets
  sprite shading SHADING_MIM_11, the camera with SetUP_CAMERA_NO_LEAD, EnableGroup g62 off; no SetCamPerspective, no fog call).
- Paths that drop a map model or its triangles with no picture fallback (each counted by an existing census line, switch `census`):
  1. the native model path refuses the record: since MDL-5e the record is not drawn (nd_recs.c:191): `mdlw f<n> ok <a> why <code:n>`,
     `mdlfb f<n> <producer> sub <a-d> <n>`, `ndm f<n> native .. fb .. why ..` (pm_gpu.c:3529 stores the fallback; nd_model.c:243-248);
  2. the raw batch refused at the close (nd_raw_native, pm_gpu.c:1360-1389): `nrawwhy f<n> zmark .. tex1 .. unit0 .. tmpl ..` (nd_ent.c:315);
  3. a ring triangle refused (TRI-4 drop, nd_geom.c:291-293): `tridrop f<n> none .. scr .. tss .. lit .. fog .. zmark .. tex1 .. refuse`;
  4. the texture cache full (pm_tex.c:1467, :1500: "draws using it are skipped");
  5. the compiled map range skip (dlc_box_out, nd_shape.c) and the empty native hit (ndmhit empty, pm_gpu.c ndn_log).
  Which of these takes the floor is not decidable by code read (the floor model's texture, combine and aux tile are in the shape file,
  not in the source): this census decides it. A map whose floor renders (iwa_10, fixed by R13NOP) is the comparison.

## Start
`sh 3ds/tools/wt_new.sh MIM11CEN`. PM_ROW=MIM11CEN PM_LANE=MIM11CEN.

## Exact edits (tool files only)
Scenario and bench mim11_cen: form of mdl5_iwa10_cen; debug `warp=mim_11:0 census loglevel=debug det`, until `ndm f300 native`,
cap 30, deadline 25, first_marker `warp: mim_11 entry 0`, expects warp, needs_picture false. Bench azahar_stream, normal, default
false, tags azahar census. `bench.py rebuild`. Commit both files together.

## Gate (measurement)
Build (lane MIM11CEN), memcheck OK. One list, 2 Azahar starts: `bench.py run mim11_cen,mdl5_iwa10_cen --row MIM11CEN --why "which
path drops the mim_11 floor: model, batch, triangle, texture or range census"`. Hand back, for both logs, the lines of frame 300:
mdlw, mdlfb, mdlsub, ndm, ndmhit, nrawwhy, tridrop, rect, and any `tex cache full`, `texcache VRAM full`, `seamfix` line of the
map load. No reading of shots. Stop after the hand-back.

## At the end
Delete this row file in the completing commit. Hand back the lines as printed.
