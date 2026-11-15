# ws_MESH1: the animator mesh lists (Munchlesia) become device units; animator.c reads the units, not the list words (C5, C6, C7)
Model: Sonnet (user yes for design-heavy code). Why not Haiku: 4 converter files and animator.c must agree on one new rule. PM_ROW, PM_LANE = MESH1. Branch MESH1,
worktree build3ds/wt/MESH1. Budget: 1 build, 8 Azahar starts (base runs not charged). Start now. Parallel with ws_DGBJOIN (no shared file).
Owns: 3ds/tools/gen_gfx_syms.py, nd_gconv.h, nd_gfx_conv.c, nd_gtab_test.c, gset.d/MESH1.txt, azt.d/MESH1.json, bench.d/MESH1.json (new), src/animator.c,
src/entity/model/Munchlesia{BeginChew,Envelop,Grab,Reset,Reset1,SpitOut}_{gfx,anim}.c, their twins in 3ds/tools/gfxin/src/entity/model. Edit no other file.
FACT (listcen, main f495844bdf): C5 8914, C6 651, C7 1286; each of the 6 Munchlesia*_gfx.c has 138 data (828). FACT (ntab_inherit.txt): all 6 `Entity_Munchlesia*_Render`
are `itab unsup unit address outside the symbol map (unit 14)`: `gsSPVertex(Entity_MunchlesiaGrab_vertexArray, 31, 0)` loads a Vec3s array (MunchlesiaGrab_gfx.c:8, :2813).
FACT: only these 6 set sources define `^Vec3s`. FACT: their nodes have vertexStartOffset >= 0 (MunchlesiaGrab_anim.c:146-199): animator.c:899-983 copies the list each frame.
The copy stays (byte-equal). Only its word source changes to NdNDev u, nu (nd_gtab.h:116). nd_ntab_find is safe on the logic thread (sorted before it starts, nd_gtab.c:79).
## Group 1: converter rule `a VTX into a Vec3s symbol makes a mesh list: units only, no draws` (0 builds, commit `MESH1 1: ...`)
1. nd_gconv.h:4 Before `GS_LIT /* LIT1: a Lights1 object */ };` After `GS_LIT /* LIT1: a Lights1 object */, GS_V3S /* MESH1: a Vec3s array */ };`.
   gen_gfx_syms.py:14 Before `^(Gfx|Vtx)\s+` After `^(Gfx|Vtx|Vec3s)\s+`; :104 add `"Vec3s": "GS_V3S"` to the dict. nd_gfx_conv.c:935 after the GS_LIT line add `else if (s->kind == GS_V3S) printf("extern Vec3s %s[];\n", s->name);`.
2. nd_gfx_conv.c:216 after `unsigned lit; ...` add `unsigned mesh; /* MESH1: a VTX unit into a Vec3s symbol (animator.c reads the units) */`.
   :444-447 Before `} else if (!v2_sym_expr(cx->ue[i], sizeof cx->ue[i], (uintptr_t)w1, kind)) {` `v2_unsup("unit address outside the symbol map", k);` `return;` `}`
   After the same `} else if (...) {` then `if (kind == GS_VTX && v2_sym_expr(cx->ue[i], sizeof cx->ue[i], (uintptr_t)w1, GS_V3S)) cx->mesh = 1;`
   `else { v2_unsup("unit address outside the symbol map", k); return; }` `}`. :700-701 Before `v2_walk(g, 0, 0);` `v2_flush();` After `if (!cx->mesh) { v2_walk(g, 0, 0); v2_flush(); }`.
3. nd_gtab_test.c before :606 add `static unsigned v2_n_mesh;` and `static int v2_mesh(const void* list)`: 1 when the GS_GFX symbol of list has, before its NDO_END, an NDO_VTX
   unit whose w1 is inside a GS_V3S symbol (the loop of v2_units, :506-512). :625-626 Before `m_walk(g1);` `v_replay(t, fault);`
   After `if (v2_mesh(t->list)) v2_n_mesh++; else { m_walk(g1); v_replay(t, fault); }`. After :650 add `printf("ntab mesh %u\n", v2_n_mesh);`.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/mesh1_h1.txt 2>&1`: PASS, `ntab mesh 6`, `bad 0` on `gconv lists` and both later `ntab tables` lines. GUESS gset ok 1461 -> 1467. Commit.
## Group 2: animator.c reads the units (0 builds, commit `MESH1 2: ...`)
5. animator.c after :6 add `#include "nd_gtab.h"` and `#include "pm_port.h"` (pm_log_evt :17, pm_frame_count :61).
6. :900-924 Before `Gfx* gfxPos; s32 vtxIdx, dlIdx; s32 j = 0; s32 k;`, the END scan block, `j++;`. After:
   `const NdNDev* mesh = nd_ntab_find(node->displayList); /* MESH1: the device units of the list (a moved list's key is its entry) */`
   `const uint32_t* units = mesh != nullptr ? mesh->u : (const uint32_t*)node->displayList; s32 nu = mesh != nullptr ? (s32)mesh->nu : 0;`
   `Gfx* gfxPos; s32 vtxIdx, dlIdx; s32 j; s32 k;` `if (mesh == nullptr) while ((units[2 * nu] >> 24) != NDO_END) nu++;` `j = nu + 1;`
   `{ static int s_mesh_log; if (!s_mesh_log) { s_mesh_log = 1; pm_log_evt("anim mesh f%u units %d dev %d", pm_frame_count(), (int)nu, mesh != nullptr); } }`
7. :940-952 Before `do { w0 = ((s32*)node->displayList)[dlIdx++]; w1 = ...; { /* HC4e2b */ ... } cmd = w0 >> 0x18; if (cmd == NDO_END) { break; }`
   After `for (dlIdx = 0; dlIdx < nu; dlIdx++) { w0 = units[2 * dlIdx]; w1 = units[2 * dlIdx + 1]; cmd = w0 >> 0x18;`. :980 `} while (true);` becomes `}`. :953-979 stay. Host as step 4. Commit.
## Group 3: move, bench, gate (1 build, 5 charged runs, commit `MESH1 3: ...`)
8. `python 3ds/tools/listcen.py > build3ds/out/mesh1_lc0.txt`. gset.d/MESH1.txt: the 6 `src/entity/model/Munchlesia*_gfx.c` paths. `python 3ds/tools/gset.py 3ds/tools/gset.d/MESH1.txt
   build3ds/out/gset_ok.txt > build3ds/out/mesh1_set.txt 2>&1`: last line `gset ok`. Precedent (MunchlesiaChewing_anim.c:8): `.displayList = NDN(Entity_MunchlesiaChewing_Render),`. Host: PASS, moved 1388 -> 1394 (GUESS).
9. azt.d/MESH1.json: a copy of sbk14_drec (azt.d/GOLDENT1.json:29-51), name `jan09_mesh_drec`, debug `warp=jan_09:0 loglevel=debug det drec=200:2`, first_marker and warp regex
   `warp: jan_09 entry 0`. bench.d/MESH1.json: a copy of bench.d/GOLDENT1.json sbk14_drec with that name. FACT: jan_09 makes Entity_Munchlesia (jan_09/entity.c:6).
10. One build: rc 0, memcheck margin above 0 KB. For B in jan09_mesh_drec spot_mac01_drec sbk14_drec fxr_drec fxp_drec: `bench.py base $B --row MESH1 --commit $(git merge-base HEAD main)`,
   `bench.py run $B --row MESH1 --why "MESH1 3"`, `bench.py cmp $B --row MESH1 --commit $(git merge-base HEAD main)`. Pass: byte-equal, and the jan09 head log has
   `anim mesh f\d+ units \d+ dev 1` (seeded check: no line or `dev 0` fails). listcen > mesh1_lc1.txt (GUESS C5 -828, C6 -6, C7 -12). Delete this page and its INDEX.md line. Commit.
Fix rights: compile errors in owned files; a `gset keep` line (edit it by hand to the precedent form). A cmp difference, a failed base run or no anim mesh line: write the output
to build3ds/out/, send the path to the oracle (CLAUDE.md 4, 5). Never change a base. Stop: group 3 gated and committed. Hand-back build3ds/out/handback_MESH1.txt, first 8 lines:
commit hashes, gconv lists and gset ok (before, after), ntab mesh, the 5 cmp results, the anim mesh line, C5 C6 C7 (lc0, lc1), memcheck margin, Azahar starts used.
