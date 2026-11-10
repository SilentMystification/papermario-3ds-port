# Row STEREOFX1: no NULL write in the stereo record; present path in step

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh STEREOFX1`; PM_ROW=PM_LANE=STEREOFX1 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/src/pm_gpu.c.
## Exact edits
1. Replace the two pointers (:1961-1962) with `static gxCmdEntry_s sr_ent[SR_MAX]; static SrFb sr_fb[SR_FB_MAX];` (8.6 KB static, under the tens-of-KB heap line of hw rule 7). Delete the malloc line and the NULL test in sr_frame_begin. Fix the comment.
2. pm_gpu_menu_present_w: `stereo_frame = targetR && u_eye >= 0 && eye_slider > 0.02f;` and `sr_on = 0;` before C3D_FrameBegin.
3. sr_replay copy arm: after `e.data[1] = sr_map(e.data[1]);` add `e.data[2] = sr_map(e.data[2]); /* STEREOFX: a copy into the left target goes to targetR */`. Change the skip test to `if (!sr_vram(e.data[1]) && !sr_vram(e.data[2])) continue;`.
## Gate
Build OK. memcheck static margin OK. Run merlee_stereoat (cap 45): "stereo: replay" lines with over 0, until-line reached. Byte gate: `bench.py cmp spot_mac01_det` (mono, unchanged). Then the orchestrator pushes to the XL for the user's two cases.
