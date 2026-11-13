# Row NXF1: nd_xf_push and nd_xf_pop, the native transform calls (one NDC_NAPI record each, as nd_val.c)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main ec2519608 with CODEMOD)
- Blocker of plan v3 (NPSO3) is not needed: a transform record holds no pipeline state. The pattern exists: 3ds/src/nd_val.c
  records one typed op unit per call (`nd_rec_napi(nd_val_run, u, 1u)`) and its runner calls nd_walk_state, which runs the unit
  with the walker's own code. For a matrix that is nd_walk.c:455-459: NDO_MTX -> load_mtx(a, b, 0), NDO_POPMTX -> gbi_popmtx(b).
- The typed unit of a list word: nd_op_xlate(w0, w1, out[2]) (nd_op.c:13, :52-53): G_MTX -> NDO_W0(NDO_MTX, w0 & 0xffffff), w1;
  G_POPMTX -> NDO_W0(NDO_POPMTX, ...), w1. NDO_W0(id, a) = id << 24 | a & 0xffffff (nd_op.h:54).
- F3DEX2 words (include/PR/gbi.h:1754-1755, :1736-1742, :2803): gSPMatrix(pkt, m, p) = gDma2p(G_MTX, m, 64, p ^ G_MTX_PUSH, 0):
  w0 low 24 bits = 0x380000 | ((p ^ 1) & 0xff), w1 = m. gSPPopMatrix(pkt, n) = gSPPopMatrixN(pkt, n, 1) = gDma2p(G_POPMTX, 64,
  64, 2, 0): w0 low 24 bits = 0x380002, w1 = 64. The n of gSPPopMatrix is not in the words.
Start: `sh 3ds/tools/wt_new.sh NXF1`. PM_ROW=NXF1 PM_LANE=NXF1 on every call. No blocker.
## Touch only (places)
New files 3ds/include/nd_xf.h, 3ds/src/nd_xf.c, 3ds/tools/nd_xf_test.c, 3ds/tests/host.d/nxf.sh. Nothing else.
## Exact edits
1. nd_xf.h (guard ND_XF_H, stdint.h): comment lines with the mapping
   `gSPMatrix(gMainGfxPos++, m, p) -> nd_xf_push(m, p)` and `gSPPopMatrix(gMainGfxPos++, n) -> nd_xf_pop(n)`, then
   `void nd_xf_push(const void* m, unsigned p);` `void nd_xf_pop(unsigned which);` `void nd_xf_run(const uint32_t* rec);`
2. nd_xf.c (includes as nd_val.c: stdint.h, "ultra64.h", "nd_op.h", "nd_api.h", "nd_xf.h"):
   - nd_xf_run(rec): `nd_walk_state(rec, 1u);`
   - nd_xf_push: `const uint32_t u[2] = { NDO_W0(NDO_MTX, 0x380000u | ((p ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)m };
     nd_rec_napi(nd_xf_run, u, 1u);`
   - nd_xf_pop: `(void)which; const uint32_t u[2] = { NDO_W0(NDO_POPMTX, 0x380002u), 64u }; nd_rec_napi(nd_xf_run, u, 1u);`
3. nd_xf_test.c (host): defines `nd_rec_napi` (copies the units of the last call into a test array), `nd_walk_state` (empty)
   and `uint32_t f32tof24(float vf)` (correction 1: nd_tmpl.c calls it. Copy the 11 lines of 3ds/tools/nd_cmd_test.c:16-26, as
   nd_cmd_test, nd_emit_test, nd_gv_test, nd_model_test and nd_tmpl_test do) for the test link. For each p in {0, 1, 2, 3, 4, 5, 6, 7} and m = (void*)0x00123450: build `Gfx g; gSPMatrix(&g, m, p);`, run
   nd_op_xlate(g.words.w0, g.words.w1, want), call nd_xf_push(m, p), compare the 2 recorded words with want. The same for
   `gSPPopMatrix(&g, G_MTX_MODELVIEW)` against nd_xf_pop(G_MTX_MODELVIEW). Print `nxf ok 9/9` or `nxf FAIL <case>` and exit 1.
4. host.d/nxf.sh (LF), the flags and link set of nd_host_test.sh:41 (nd_cmd_test links nd_op.c the same way. /tmp/ndcomb.o is
   built at :30, before the host.d loop): `gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US
   -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nxf 3ds/tools/nd_xf_test.c
   3ds/src/nd_xf.c 3ds/src/nd_op.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm && /tmp/nxf`.
5. Commit: `NXF1: nd_xf_push and nd_xf_pop as NDC_NAPI typed-unit records, host test against the gSPMatrix words`.
## Gate (expected values)
1. Seeded fault (a value change): in nd_xf_push write `(p ^ 0u)` for `(p ^ 1u)`. `python 3ds/tools/bench.py run nd_host --row NXF1
   --why "NXF1 seeded fault"`: expected 8 FAIL lines `nxf FAIL push p=<0..7>` (every p flips bit 0), pop ok, nd_host FAIL.
2. Restore. Same command, --why "NXF1 gate": `nxf ok 9/9`, `ND HOST TESTS PASS`.
3. `PM_ROW=NXF1 PM_LANE=NXF1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK. No Azahar.
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
