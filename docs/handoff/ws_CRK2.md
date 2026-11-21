# ws_CRK2 (Haiku): runtime seam census (drec vertex dump, seamcen.py, bench seam_mac01_drec)
Designer: oracle NATIVE24. Worktree build3ds/wt/CRK2, branch CRK2. PM_ROW=CRK2 PM_LANE=CRK2. Start: main. Counter: none. Metric: the seamcen line.
Objective: count, on real device draws, the points that reach the GPU with 2 input pairs (vertex bits, palette bits). FACT: drec.txt holds the palette
words (pm_gpu.c:724) but only a hash of the vertices (nd_drec.c:43-44). With the switch off, drec.txt stays byte-equal.
## Directory order: 3ds/include/nd_drec.h, 3ds/src/nd_drec.c, 3ds/tools/nd_drec_test.c, 3ds/src/pm_gpu.c, 3ds/tools/seamcen.py (new),
3ds/tests/host.d/seamcen.sh (new), 3ds/tools/azt.d/CRK2.json (new), 3ds/tools/bench.d/CRK2.json (new). Edit no other file.
## Edits
1. nd_drec.h:18, after `const float (*mtx)[16]; uint32_t nmtx;`: add `uint8_t vx; /* CRK2: 1 writes the vx and ix lines (switch drecvx) */`.
2. nd_drec.c:55, before `fprintf(f, "end\n");`:
```c
    if (r->vx) { /* CRK2 (switch drecvx): the vertex bytes (16 per token) and the index bytes (2 per token) as hex */
        static const char hx[] = "0123456789abcdef"; const unsigned char* b = (const unsigned char*)r->vtx.p;
        fputs("vx", f); for (i = 0; i < r->vtx.n; i++) { if (!(i & 15u)) fputc(' ', f); fputc(hx[b[i] >> 4], f); fputc(hx[b[i] & 15u], f); }
        b = (const unsigned char*)r->idx.p;
        fputs("\nix", f); for (i = 0; i < r->idx.n; i++) { if (!(i & 1u)) fputc(' ', f); fputc(hx[b[i] >> 4], f); fputc(hx[b[i] & 15u], f); }
        fputc('\n', f);
    }
```
3. nd_drec_test.c: a 4th record `build(&d, 3, vtx); d.vx = 1;` after c. Append to expect the draw 0 block as `draw 3`, and after its m line the lines
   `vx 000306090c0f1215181b1e2124272a2d 303336393c3f4245484b4e5154575a5d 606366696c6f7275787b7e8184878a8d` and `ix 0000 0100 0200`. Print `drec ok 4`.
4. pm_gpu.c: `static int drec_vx;` beside drec_on (:601). In drec_begin, in the `if (drec_win == -2) {` block (:654): `drec_vx = PMDL("drecvx");`.
   Before (:734): `rec.seq = drec_seq++; rec.kind = 1; rec.n = d->nidx; rec.regs = r; rec.nregs = n;` After: the same line plus ` rec.vx = (uint8_t)drec_vx; /* CRK2 */`.
5. seamcen.py: `seamcen.py <bench> --row <id>` (row drec.txt as drecsum.py:47-51), `--base <bench>` (drecsum.py:36-39), `--file <path>`, `--selftest`.
   Parse: blocks `draw` to `end`. Line `r <reg> f <val>` with 0x1000 <= reg < 0x1200: palette k = (reg - 0x1000) >> 4, word j = reg & 15 (float bits).
   Vertex: `struct.unpack('<4h', b[0:8])` = x y z k, only the vertices the ix indices name. Skip a draw without palette words or without a vx line.
   Reference in double, row r of palette k: c = P[4r] + P[4r+1] z + P[4r+2] y + P[4r+3] x (rows 0, 1, 3). Skip w < 0.05. sx = (cx / w * 0.5 + 0.5) * 240, sy the same with 400.
   Cluster: cell (floor(sx * 64), floor(sy * 64)). Join a cluster of the 9 cells around when |dsx| < 1/256, |dsy| < 1/256 and |dw| < 1e-5 w. Member key = (x, y, z, the 16 palette bits). shared = clusters of 2 or more draws. split = shared clusters with 2 or more member keys.
   f24 model (nd_seam_test.c:37-50): f32(x) = struct 'f' round trip after each operation. rn: u = bits, if u & 0x7fffffff: u = (u + 0x3f + ((u >> 7) & 1)) & 0xffffff80.
   rz: u & 0xffffff80. dp4: acc = 0, per k: acc = f24(f32(acc + f24(f32(f24(row[k]) * f24(v[k]))))), v = (1, z, y, x). sx = f32(f32(f32(f32(cx / cw) * 0.5) + 0.5) * 240).
   snap_rn, snap_rz: split clusters whose (X16, Y16) differ, X16 = int(f32(f32(sx * 16) + (-0.5 if sx < 0 else 0.5))). maxd16 = max(|dsx|, |dsy|) * 16 with rn. Line: `seamcen <name> draws %d verts %d clusters %d shared %d split %d snap_rn %d snap_rz %d maxd16 %.3f`.
   Selftest (no file): rows [0,0,0,1] [0,0,1,0] [0,1,0,0] [300,0.5,0,0] (memory order w z y x), vertices (10,20,30) (40,-20,30) (-30,5,60). Case A: 2 draws,
   the same words: `shared 3 split 0`. Case B (seeded fault): draw 1 has x - 100 and row 0 word 0 = 100: `shared 3 split 3`. Both right: `seamcen selftest ok`, exit 0, else exit 1.
6. host.d/seamcen.sh: `python3 3ds/tools/seamcen.py --selftest || exit 1` (header comment as drec.sh). 7. azt.d/CRK2.json, bench.d/CRK2.json: copies of the
   HOSTCORE2 files, name seam_mac01_drec, debug `warp=mac_01:0 pos=-230:0:-250 prof=4 loglevel=debug det drec=200:1 drecvx`, event `drec f200 draws [1-9]\d* bytes [1-9]\d*`.
## Tests (output to build3ds/out/ of the worktree, read the last 20 lines)
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS with `drec ok 4`, `drec seed ok`, `seamcen selftest ok`. 2. `PM_LANE=CRK2 sh 3ds/build.sh`, then `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row CRK2 --why "drecvx off"`, `bench.py cmp spot_mac01_drec --row CRK2`: byte-equal.
4. `bench.py run seam_mac01_drec --row CRK2 --why "seam census baseline"`, then `python 3ds/tools/seamcen.py seam_mac01_drec --row CRK2`: one seamcen line.
Fix rights: the 8 files. A cmp DIFF, a selftest FAIL or a seamcen line with draws 0: send the output path to oracle NATIVE24.
Stop: `CRK2 1: gate` commit with this page deleted. Hand-back: one SubagentHandback: commit hash, host result, c0gate, cmp result, the seamcen line.
