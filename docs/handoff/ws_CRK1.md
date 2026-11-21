# ws_CRK1 (Haiku): seam census step 6 (mac_01 camera, f24 projection, 1/16 snap, holes with every list)
Designer: oracle NATIVE24. Worktree build3ds/wt/CRK1, branch CRK1. PM_ROW=CRK1 PM_LANE=CRK1. Start: main after `Merge CRK0` (WAIT). Counter: none. Metric: step 6 line.
Objective: give the cracks of plan_seams.md section 3 a size in pixels. FACT: step 4 projects all lists with one matrix (nd_seam_test.c:237-243, :392).
The device projects each list with its own palette (nd_mtx.c:112, :132). No device file changes: no bench, no Azahar run.
## Directory order: 3ds/tools/nd_seam_test.c, then 3ds/tools/nd_host_test.sh. Edit no other file.
## Design (nd_seam_test.c; it uses the CRK0 records C5V and c5m of mac_01)
1. Move `static unsigned cnt[64]; static float post[64][16], game[64][4][4];` (:353) above ndref_camera as cam_cnt, cam_post, cam_game. ndref_camera
   gets `int* best_o` and writes best there. Before (:392): `if (cam && !ndref_camera(toad, o))` After: `if (cam && !ndref_camera(toad, o, &cam_best))`.
2. Step 6 runs at the end of seam_step5 for mac_01 when cam is 1 (add a cam argument). Per table list li: game = nd_mtx_mul(c5m[li], cam_game[cam_best]) (nd.h:37), c6p[li] = nd_mtx_pcomp(cam_post[cam_best], game) (nd.h:48).
   mtxchk: count lists of cls 1 or 2 whose game equals (float ==, all 16) one cam_game[g] with cam_cnt[g] > 0.
3. Projection with a rounding mode (rz 0: round to nearest even = fp24 :37; rz 1: round toward zero) and the object vertex:
```c
static float fp24z(float x) { union { float f; uint32_t u; } v; v.f = x; v.u &= ~0x7fu; return v.f; }
static float dp4_m(const float row[4], const float v[4], int rz) { /* rz 0 is dp4_fp24 (:46) bit for bit */
    float acc = 0.f;
    for (int k = 0; k < 4; k++) { float p = rz ? fp24z(fp24z(row[k]) * fp24z(v[k])) : fp24(fp24(row[k]) * fp24(v[k])); acc = rz ? fp24z(acc + p) : fp24(acc + p); }
    return acc;
}
static int project_v(const float o[16], const int16_t s[3], int rz, float* sx, float* sy, float* iw) { /* project() :237, object vertex, list palette */
    float v[4] = { 1.f, (float)s[2], (float)s[1], (float)s[0] };
    float cx = dp4_m(o + 0, v, rz), cy = dp4_m(o + 4, v, rz), cw = dp4_m(o + 12, v, rz);
    if (cw < 0.05f) return 0;
    *sx = (cx / cw * 0.5f + 0.5f) * RW; *sy = (cy / cw * 0.5f + 0.5f) * RH; *iw = 1.0f / cw;
    return !(*sx < -2048.f || *sx > 2048.f || *sy < -2048.f || *sy > 2048.f);
}
```
4. Snap as raster() :299: X16 = (int64_t)(sx * 16.f + (sx < 0 ? -0.5f : 0.5f)), Y16 the same. Per shared group (CRK0 step 5), all record pairs of other li,
   both on screen: snap_rn++ (once per group) when (X16, Y16) differ with rz 0, snap_rz++ the same with rz 1. maxd16 = max(|dsx|, |dsy|) * 16 with rz 0.
   same++ per pair with equal v bits and equal c5m bits whose float sx or sy bits differ under the mode rm[li] of each list (rm 0 for all). It must be 0.
5. Gap pairs (CRK0 sweep), both on screen (0 <= sx < RW and 0 <= sy < RH), rz 0: d = max(|dsx|, |dsy|). gpx++ when d > 0.0625f. gpxmax = max d.
6. raster5: copy raster() (:289-343). Per triangle use T5 {int16_t v[3][3]; uint16_t li;} (filled in the CRK0 compile loop, every class) and
   project_v(c6p[li], v[c], rz, ...) in place of project(o, ctri[k].w[c], ...). Drop the tseg loop. holes_rn = raster5(0), holes_rz = raster5(1).
7. Line, strcmp with static const char exp6[]; a difference prints `FAIL: mac_01 step 6` and the line:
   `seam step 6: mac_01 mtxchk %u of %u shared %u split %u snap_rn %u snap_rz %u maxd16 %.3f same %u gpx %u gpxmax %.3f holes_rn %u holes_rz %u tri %u skip %u`
8. Seed: getenv("SEAM_SEED") equals "6": rm[li] = 1 for every ok cls 0 list with an odd table index li. Then same > 0 and the line differs. 9. nd_host_test.sh: after the CRK0 seed line, add the same line with SEAM_SEED=6, /tmp/ndseam6.txt and `grep -q "FAIL: mac_01 step 6"`.
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines)
1. Run 1: `sh 3ds/tools/nd_host_test.sh` (timeout 600000). It fails on the empty exp6. Copy the step 6 line into exp6. Commit `CRK1 0`.
2. Run 2: host PASS. Both seeded runs exit 1. same 0 in the unseeded line. mtxchk at least 1. Build and c0gate once at the gate commit (`PM_LANE=CRK1 sh 3ds/build.sh`, `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0).
Fix rights: nd_seam_test.c and the one host script line. mtxchk 0, same above 0 unseeded, or a seeded run that passes: send the output path to oracle NATIVE24.
Stop: `CRK1 1: gate` commit with this page deleted. Hand-back: one SubagentHandback: commit hash, the step 6 line, both seeded lines, host result.
