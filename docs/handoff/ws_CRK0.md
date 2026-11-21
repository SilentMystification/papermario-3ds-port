# ws_CRK0 (Haiku): seam census step 5 in the host test (world points through the device model matrices)
Designer: oracle NATIVE24. Worktree build3ds/wt/CRK0, branch CRK0. PM_ROW=CRK0 PM_LANE=CRK0. Start: main. Counter: C1..C12 none. Metric: step 5 lines.
Objective: count the points where 2 map lists meet with 2 numeric paths (plan_seams.md section 3). FACT: step 4 skips every cls 2 list
(3ds/tools/nd_seam_test.c:215). mac_01 has 44 cls 2 lists of 148. No device file changes: no bench, no Azahar run.
## Directory order: 3ds/tools/nd_seam_test.c, then 3ds/tools/nd_host_test.sh. Edit no other file.
## Design (nd_seam_test.c, new code above seam_steps34 at :376)
1. Copy nd_seam.c:255 (SXf), :265-286 (mtx_read), :287-297 (xf_cat), :259-262 (at) and :391-410 (node_walk) as static c5_ functions. c5_at takes
   (buf, size) with base = (uint32_t)(uintptr_t)buf. Root: rd32(buf), identity SXf (nd_seam.c:412-420). Do not follow ND_FS_CALL.
   Before (nd_seam.c:409): `if (d && rd32(d)) list_add(w, rd32(d), x, 0);` After (copy): `if (d && rd32(d)) c5_add(rd32(d), x);`
2. c5_add keeps the first SXf of each addr in c5_tab[1024]. A second SXf of the same addr with other f bits (memcmp) sets its multi flag.
3. Per table list L (t.list[i], L->ok): no c5_tab entry: notx++. multi: multi++. Skip both. c5 cls != L->cls: cmis++ (must be 0).
4. Compile L as compile_soup does (:203-213, use_tp 1 for every class, also cls 2). Per index corner (:218 gives v), push one record:
```c
typedef struct { float w[3]; int16_t v[3]; uint16_t li; uint8_t cls; } C5V;
static float c5m[1024][4][4]; /* device model matrix of table list li: guMtxF2L (pm_gu.c:44 truncation), unpack (nd_mtx.c:38) */
static void c5_mv(const float f[4][4], float mv[4][4]) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) mv[i][j] = (float)(int32_t)(f[i][j] * 65536.f) * (1.f / 65536.f);
}
static void c5_put(C5V* r, const int16_t* v, unsigned li, int cls) { /* world point in float, the order of nd_seam.c:350 */
    for (int q = 0; q < 3; q++) r->w[q] = (float)v[0] * c5m[li][0][q] + (float)v[1] * c5m[li][1][q] + (float)v[2] * c5m[li][2][q] + c5m[li][3][q];
    memcpy(r->v, v, 6); r->li = (uint16_t)li; r->cls = (uint8_t)cls;
}
static int c5_cmp(const void* a, const void* b) { const C5V* x = a; const C5V* y = b; int c = memcmp(x->w, y->w, 12); return c ? c : (x->li > y->li) - (x->li < y->li); }
```
5. Sort with c5_cmp. Group = equal w bits. pts++ per group. shared++ when the group has 2 or more li. split++ when 2 of its lists have other c5m bits
   (memcmp 64). s[a][b]++ (a <= b: the classes of the first list and the first list with other bits). Keep one record per (w bits, li): array u.
6. Gap: sort u by w[0] (float compare). For i, then j > i while u[j].w[0] - u[i].w[0] <= 1.5f: other li and 0 < d2 <= 2.25f gives gap++,
   gc2++ (a cls 2 side) or gint++, and the max d2. gapmax = sqrtf(max d2).
7. Line (one per map), compare with strcmp to static const char* const exp5[6] (map order of nd_host_test.sh:274). No entry or a difference: print
   `FAIL: <map 6 chars> step 5` and the line, fail = 1. Line format:
   `seam step 5: %.6s lists %u notx %u multi %u cmis %u pts %u shared %u split %u s00 %u s01 %u s02 %u s11 %u s12 %u s22 %u gap %u gc2 %u gint %u gapmax %.3f`
8. Seed: getenv("SEAM_SEED") equals "5": for mac_01 only, set c5m[li][3][0] = nextafterf(0.f, 1.f) for every ok cls 0 list with an odd table index li, before item 4. Then s00 and split rise.
9. Call it in seam_steps34. Before (:405): `        nd_seam_free(&c1);` After: `        if (seam_step5(&t, nm, buf, size)) fail = 1;` then `        nd_seam_free(&c1);`
10. nd_host_test.sh after :274 (the /tmp/ndseam line), one line, same arguments: `if SEAM_SEED=5 /tmp/ndseam <the same arguments> > /tmp/ndseam5.txt; then echo "FAIL: seam seed 5 passed"; exit 1; fi; grep -q "FAIL: mac_01 step 5" /tmp/ndseam5.txt`
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines)
1. Run 1: `sh 3ds/tools/nd_host_test.sh` (timeout 600000). It fails on the empty exp5. Copy the 6 printed step 5 lines into exp5. Commit `CRK0 0`.
2. Run 2: host PASS. The seeded run exits 1. cmis 0 on all 6 maps. pts > 0 on all 6 maps. Build and c0gate once at the gate commit (`PM_LANE=CRK0 sh 3ds/build.sh`, `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0).
Fix rights: nd_seam_test.c and the one host script line. A cmis above 0 or a seeded run that passes: send the output path to oracle NATIVE24.
Stop: `CRK0 1: gate` commit with this page deleted. Hand-back: one SubagentHandback: commit hash, the 6 step 5 lines, the seeded line, host result.
