# design_producer_models: producer-side records for static plain models (G_PM_MODEL)

Branch nds1, base fcb281fdb. Read-only on source. One document. No Azahar start (the shape data was read from repo assets with a host script, so no console or emulator run was needed).

Tags: **[C file:line]** read in source. **[M]** measured by the host script over the repo asset. **[I]** inferred. **[D doc]** from a handoff note, not re-checked.

Host scripts (scratch, not committed): `C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\bf7242ad-7188-4cf3-8ec3-af3ba8c281b2\scratchpad\pmod\walk_shape.py` and `leaf.py`.

---

## 0. Findings that change the plan

1. **The bake exists already.** `dlc_compile` (pm_dlc.inc.c:145) and `dlc_static_build` (pm_dlc.inc.c:219-268) answer blockers 1 and 2. They copy each G_VTX once into the static vertex area (`pm_gpu_gstat_alloc`, pm_gpu.c:2251), set flag 0, and build the triangle index list with absolute slot numbers. They also split the list at G_GEOMETRYMODE (pm_dlc.inc.c:195 records `dlc_sgop`). Reuse them. Do not write a second bake.
2. **DLC is on by default.** `dlc_enabled()` is `!dd_on() && !xt_on() && !rmlog_on()` (pm_dlc.inc.c:40-42), and `dlc_on = dlc_enabled()` runs every frame (pm_gbi.c:3894). Static geometry of accepted display lists already goes through `pm_gpu_grange` (pm_dlc.inc.c:283, pm_gpu.c:2278). So the producer design does not add geometry. It removes the per-group walker state work and the per-batch `flush_draw` state (section 6).
3. **Vertex bytes need no conversion.** `fix_shape` swaps the six u16 fields of every G_VTX target at load (pm_map.c:97-128 via `swap_verts`, pm_map.c:74-84). The ring copy is `memcpy` only (pm_gpu.c:2328-2334). The attribute loader reads 4 shorts (ob[3] and the flag at +6), 2 shorts (tc), and 4 bytes (cn) (pm_gpu.c:1360-1363). So the shape bytes already have the ring layout.
4. **The flag field is the palette index.** The ring writes the palette index at +6 (pm_gpu.c:2120, 2228). A static vertex must carry flag 0 and the batch palette must hold the model's matrix at entry 0 (pm_gpu.c:2269-2270). [M] Flag is 0 in all 15001 vertex reads of the leaf display lists of mac_01 (counted per walk, nested lists counted again).
5. **No zero-copy from the shape.** `gMapShapeData` is a plain 256 KB static array (pm_map.c:35), not linear memory. `BufInfo_Add` takes one base address for the whole buffer (pm_gpu.c:1146). [I] A contiguous physical range is not guaranteed for that array. The gring and gidx buffers use `linearAlloc` (pm_gpu.c:1347-1348). Use a one-time copy per map into the static area. That copy is not per frame.
6. **Correction to agent_nds1.md (N4 2b note, "pcomp_eye does not exist").** The eye 2 palette is not `nat_mtx[1] = nat_mtx[0]`. The eye shift is in `combined`, built in `upload_mvp` (pm_gbi.c:455-464). `gv_matrix_id` stores `combined` in `gm_tab` (pm_gbi.c:1499). `gv_post` is not eye-shifted (pm_gpu.c:224-225). So the palette of eye e is `gv_post x combined_e`, and it differs by eye. The two-walk path gets the eye 2 values from the second walk (pm_nusys.c:81). A single-walk design must compute `combined` for eye 2 at the group.

---

## 1. Static shape facts (Toad Town map shapes, from the repo assets) [M]

Source files: `assets/us/mapfs/geom/mac_01_shape.bin` (112,848 bytes) and `mac_02_shape.bin`. These files are already big-endian decoded shapes with the `SHAPE_BASE` 0x80210000 pointer base (pm_map.c:17). The walker is host Python in the scratch folder.

| Shape | Nodes with a display list | Leaf DLs eligible (no MTX, flag 0, slots live, pointers in shape) | DLs with MTX or POPMTX | G_QUAD ops | Nonzero flag | Pointer outside shape |
|---|---|---|---|---|---|---|
| mac_01 | 206 | 168 | 38 | 0 | 0 of 15001 vertex reads | 0 |
| mac_02 | 141 | 131 | 10 | 0 | 0 of 9510 vertex reads | 0 |

- Ops in eligible DLs: VTX, TRI1, TRI2, GEOMODE, PIPESYNC, and nested DL (58 DLs in mac_01 contain nested G_DL). No TEXTURE, SETCOMBINE, OTHERMODE, TILE, or colour op appears inside a model DL. [M]
- Every G_TRI index refers to a slot loaded earlier in the same DL. [M]
- Vertex counts above are per walk. Nested lists are walked again, so they are not unique counts. The unique vertex data is at most the shape file size.
- The 38 DLs with G_MTX (mac_01) are child nodes with a static transform. Exclude them in v1. This is the same refusal as DLR_OP in dlc_compile (pm_dlc.inc.c:201).

---

## 2. Group anatomy: what model.c emits for a plain opaque model

Plain test: `renderClass` is 1CYC or 2CYC, `tintCombineType` is NONE, and the flags have no CUSTOM_GFX, TEX_PANNER or BILLBOARD (model.c:1500-1503). Fog on this frame makes the class FOG, so the model is not plain (model.c, fog block before the tint switch). The record bit (header w1 bit 9) is set only for SURFACE_OPA (model.c:1520 area). Keep that rule for v1.

Group bounds:
- Header op: `mgHdr = (*gfxPos)++` (model.c:1514). `w0 = G_PM_MODEL << 24`, `w1` = pmSlot bits 0-7, static matrix bit 8, plain bit 9, pmMtxVer bits 16-31 (model.c:1516-1520).
- Count: `mgHdr->words.w0 = G_PM_MODEL<<24 | (gfxPos - mgHdr - 1)` at model.c:2073. The group is the n words after the header.
- Walker entry: `case G_PM_MODEL` (pm_gbi.c:3956-3966) calls `mg_run(pc, n, &nx, w1)` (pm_gbi.c:3830).

Ops of a plain group, in emit order:

| # | Op | model.c line | Value source | Known at bake | Emit source |
|---|---|---|---|---|---|
| 1 | G_PIPESYNC | 1524 | static | yes | template |
| 2 | G_MOVEMEM lights (gSPSetLightsN, up to 8) | 1531-1552 | `groupData->lightingGroup` pointer; contents may change per frame | pointer | walker in v1 (`gbi_movemem`, pm_gbi.c:3513-3530 fills lt[]) |
| 3a | G_PM_TEX (w0 = handle index 0..127, w1 = handle gfx) | 1574 or 1578 (`MODEL_TEX_OP`, model.c:185) | `TextureHandles[model->textureID + variation]` | index and gfx pointer | resolve at emit (section 7) |
| 3b | no texture: G_TEXTURE off, combine SHADE, dither x2 | 1582-1585 | static | yes | template |
| 4 | G_PM_RM (w1 = `ModelRenderModes[renderModeIdx]`, model.c:190) | 1687, 1751, 1815, 1824, 1950, 1988 | static per renderMode and class | yes (list pointer) | template key |
| 5 | G_MTX finalMtx (load or mul, push flag) | 1994, 2005 | pointer `model->finalMtx`; contents per frame | pointer and mode bits | matrix value at emit (section 5) |
| 6 | G_DL `modelNode->displayData->displayList` | 2056 | static (shape) | DL pointer | native draw (section 4) |
| 7 | G_POPMTX | 2068 | static | yes | walker in v1 |
| 8 | G_PIPESYNC | 2071 | static | yes | template |

Fog and tint ops (model.c:1483-1494, 1816-1817, 1884-1886, 1960-1961) do not occur in plain groups. The walker's combine and othermode state comes only from the G_PM_RM list (row 4) and the no-texture branch (row 3b). [I] Colour ops (prim, env) do not occur in plain groups. This is an inference from the lists in model.c:190 and the emit code. It is not checked line by line (open item O2).

**What the group already names by pointer.** Rows 3a, 4, 5, 6 and 2 each carry a pointer. So the bake key of a group can be the tuple (DL pointer, render-mode list pointer, texture index, texture gfx pointer, light pointer, matrix mode bits). No new header field is needed. The pointers are read from the group words the walker already sees (pm_gbi.c:3830 onward).

**Walker state at group start [I].** The combine, othermode, geometry, tile and colour state carry over from the previous draw. A plain group resets them through rows 3a, 3b and 4. The geometry state is the exception: a GEOMODE op in the DL may depend on the incoming bits (section 3, field `dep`).

---

## 3. Data structures (proposed names)

```c
/* one geometry segment of a model DL (cut at G_GEOMETRYMODE, from dlc_sgop) */
typedef struct {
    u16 tri0, ntri;      /* triangle range in NatModel.idx (3 indices each) */
    u32 geom;            /* geometry mode after the segment (known at bake) */
    u32 dep;             /* incoming geometry bits the segment reads (0 when none) */
    u8  tmpl;            /* template id, 0xff until captured */
} NatSeg;

/* one static model DL (keyed by DL pointer, valid for one pm_dlc_gen ^ gstat_gen) */
typedef struct {
    const Gfx* dl;       /* shape DL pointer (the key) */
    u32 gen;             /* pm_dlc_gen ^ (gstat_gen << 16), as rec_try (pm_gbi.c:3646) */
    u16 vbase;           /* first slot in the static area (GRING_VTX and up, pm_gpu.c:116) */
    u16 nv, nidx;        /* vertices copied, indices */
    u16* idx;            /* LINEAR u16 index array, absolute ring slots (base = gring) */
    u8  nseg;            /* NAT_SEG_MAX = 8; more segments refuse */
    u8  ok;              /* 1 when every check of section 8 passed */
    NatSeg seg[8];
} NatModel;

/* per-draw inputs, filled at emit, never stored */
typedef struct {
    u16 gm[2];           /* game matrix slot per eye (gm_tab index, pm_gbi.c:1480) */
    u16 tex0, tex1;      /* texture slot (pm_tex_cur_slots) */
    const Gfx* texgfx;   /* handle gfx list to run for the bind (section 7) */
} NatIn;
```

Templates: `NatTmpl` = the state words of one draw (from the N4 capture: `nat_tmpl_add` and `nat_tmpl_strip`, pm_native.c, N4 step 1). Key: (renderModeIdx, textured or not, segment geom, `use_t1`). At most `NAT_TMPL_MAX` = 64 (pm_native.h:10).

Storage: `NatModel` table of 128 entries (one per DL pointer in the map). `idx` arrays come from one linear pool allocated once (linearAlloc, like gidx at pm_gpu.c:1348). The pool resets with `pm_gpu_gstat_reset` (pm_gpu.c:2264) and `pm_dlc_gen` (pm_dlc.inc.c:7).

---

## 4. Bake procedure (once per DL and generation)

B1. Entry: the first `G_PM_MODEL` group whose DL is not in the table. Hook: `mg_run` (pm_gbi.c:3830) or the `case G_PM_MODEL` arm (pm_gbi.c:3956). Look up `NatModel` by DL pointer. A stale generation means a rebuild.

B2. Eligibility gate (section 8). If a check fails, mark `ok = 0` for that DL and leave the group on the walker. Do not retry in the same generation.

B3. Geometry: call `dlc_compile(e, dl)` (pm_dlc.inc.c:145). If it returns `DLR_OK`, call `dlc_static_build(e, dl)` (pm_dlc.inc.c:219). That fills `grp` (DlcGrp) with `vtxs` at `gstat` and `sd` indices. Change one thing: `sd` comes from a linear pool, not `malloc` (pm_dlc.inc.c:225-230). Copy the result into `NatModel.idx`.

B4. Segments: from `DlcGrp` (`ng` geometry ops, pm_dlc.inc.c:195) fill `seg[]` with `tri0`, `ntri`, and `geom` (the `dlc_sgop` pairs applied in order). Compute `dep`: a bit read before any SET or CLEAR for it in the DL is in `dep`.

B5. Vertices: as `dlc_static_build` does (pm_dlc.inc.c:225-265): `pm_gpu_gstat_alloc(nv, &dst)` (pm_gpu.c:2251), `memcpy` 16 bytes per vertex, flag set to 0, then `pm_gpu_gstat_commit(first, nv)` (pm_gpu.c:2258, which flushes the data cache with `pm_fl_dc`). This is once per map.

B6. Templates: none at bake. Templates come from the first walked draw of each key (step P2). The walker draws the first group normally and captures it (N4 path: `rec_done` calls `nat_tmpl_strip`, pm_gbi.c, see agent_nds1.md N4 step 2).

B7. Set `ok = 1`, store `NatModel`. Log one line per DL with `nseg`, `nv`, `nidx` (the count of `PM1 bake` lines is a test check, section 10).

---

## 5. Emit procedure (per group, per eye)

E1. Entry: a group whose `NatModel.ok == 1`, whose segments all have a template, and whose per-frame inputs pass (E2 to E4). Otherwise run the walker as today.

E2. Incoming geometry: for each segment, `geom_in & dep` must equal the value stored for that template. Else the group takes the walker.

E3. Texture (rows 3a and 3b): if textured, run the handle gfx list (`tb_run`, pm_gbi.c:2760, or `tex_op`, pm_gbi.c:3010) and read the slots with `pm_tex_cur_slots` (pm_gpu.c:551). If the handle is not resident, the group takes the walker.

E4. Matrix (row 5): the value `combined_e` for each eye e:
- eye 0: `combined = mv[mv_sp] x proj` (the walker's `upload_mvp` at pm_gbi.c:455-464 with `pm_eye_e = 0`).
- eye 1: the same product with the stereo shift of pm_gbi.c:455-464 (`p[r][0] += e * p[r][3]`, `p[3][0] -= e * cw`).
- `gm_e` = `gv_matrix_id(lit)` (pm_gbi.c:1480), which returns an index into `gm_tab`. For a lit segment use `need_nm = 1`.

E5. Palette: `pm_gpu_pal_fill(combined_e, gm_e, e)` (pm_gpu.c:855) writes `nat_mtx[e][gm_e]`. This is the N4 2b call. The table holds the eye-specific product (section 0, item 6).

E6. Emit each segment: `pm_gpu_grange(tab, gm, vp, uvso, cmul, cadd, zmix, ct, fx, idx + tri0*3, ntri*3)` (pm_gpu.c:2278). These are the arguments dlc_seg already passes (pm_dlc.inc.c:283). v1 uses this call and keeps the existing batch merge. v2 writes the template words and the patch words directly (step P4). The v2 draw must not call `flush_draw` per segment.

E7. Second eye: for stereo (`pm_gpu_stereo()`), run E4 and E5 with eye 1 and E6 again after `pm_gpu_eye(1)` (pm_nusys.c:81 is the place of the second walk today). v2 copies the span instead (section 6).

---

## 6. Stereo and matrices

- Today: two walks (pm_nusys.c:81). Each walk computes its own `combined` and `gm_tab`. That gives the eye values of section 0, item 6.
- Target: one walk. At each group, E4 and E5 run for both eyes. Cost per group: two 4x4 products plus one `gv_matrix_id` lookup per eye. [I] This is small compared with `flush_draw`.
- The 2-eye GPU work is not reduced by this design. Only the CPU walk is reduced. The GPU cost of the second eye is the open GPU busy probe (design_native_draw.md section 9).

---

## 7. Texture handle resolution at emit

- The handle index (w0 bits 0-6 of row 3a, `G_PM_TEX`) and the gfx pointer (w1) identify the bind. The producer does not store the N64 address or tile. The gfx list of the handle sets SETTIMG, SETTILE and LOADTLUT. `tex_op` (pm_gbi.c:3010) runs them.
- Emit calls `tb_run(texgfx)` (pm_gbi.c:2760). `tb_run` skips when the same list ran last with no texture write (TBREP, pm_gbi.c:2761-2766), so a repeated texture is free.
- Slots: `pm_tex_cur_slots(&tex0, &tex1)` (pm_gpu.c:551, used at pm_gpu.c:1053). Patch the texture address words from these.
- TLUT: a palette change in the handle list (LOADTLUT) changes the bind. The bake key includes the handle index, so a TLUT change of the same handle is caught by `tb_run` (it is a text write, `tex_gen` changes).

---

## 8. Eligibility rules

A model DL and its group qualify when all hold:
1. `dlc_compile` returns `DLR_OK` (pm_dlc.inc.c:145-205). This excludes MTX, TEXTURE, OTHERMODE, COMBINE and every colour op inside the DL.
2. Every G_VTX source lies in `gMapShapeData` (the range check in dlc_compile, pm_dlc.inc.c:153-154; the same test as the G_VTX case).
3. The flag of every copied vertex is 0 after copy (B5 sets it).
4. `nseg <= 8`, and `nidx > 0`.
5. The group is plain (model.c:1500-1503) and SURFACE_OPA (header w1 bit 9).
6. `!(flags & MODEL_FLAG_HAS_LOCAL_VERTEX_COPY)` (model.c:2054). The animator copies vertices into the heap (animator.c:70-95). Those models are not static.
7. No light change since bake for a lit segment: the light words (row 2) still run in the walker, so `lt[]` is current. No new check is needed in v1.
8. Shape DL is final: `pm_dlc_gen` unchanged (pm_dlc.inc.c:7, pm_map.c:377-379).

Not covered in v1: entity and NPC models outside the map shape [open item O3], models with G_MTX inside the DL (38 in mac_01), decal, alpha-test and translucent render modes (only SURFACE_OPA sets bit 9, model.c:1520 area).

---

## 9. Per-draw inputs versus bake-time constants

| Field | Bake | Emit | Source |
|---|---|---|---|
| Vertex base and index array | yes | no | B3, B5 |
| Triangle count per segment | yes | no | B4 |
| Geometry mode after segment | yes | checked (`dep`) | B4, E2 |
| Render mode and combine, othermode | yes (template) | no | row 4, template |
| Texture slot (tex0, tex1) | no | yes | E3 |
| Game matrix slot per eye | no | yes | E4, E5 |
| Projection per eye | no | yes | E4 (pm_gbi.c:455-464) |
| Light table `lt[]` | no | via walker | row 2 |
| Viewport, uvso, cmul, cadd, zmix, cull, fx | no | yes | `ep_p` epoch snapshot (pm_gbi.c:1515, `gv_epoch_enter` pm_gbi.c:1531) |

The `ep_p` values are the epoch parameters that dlc_seg already passes (pm_dlc.inc.c:278-283). Reuse them. Do not compute a second set.

---

## 10. Code steps (each one agent, about 25 tool calls)

Each step names its affected scenarios before it starts (test scope rule). Scenarios: `census_toad` and `census_toad_rp` (NDS1 census, 26 batches, 338 triangles, 23 distinct, from agent_nds1.md), `census_yard`, and one heavy Toad Town shot scenario.

**P1. Bake only (no draw change).** Files: `3ds/src/pm_native.c` (NatModel table, linear pool), `3ds/src/pm_dlc.inc.c` (sd from linear pool, one line), hook in `mg_run` (pm_gbi.c:3830, bake call only).
- Test: census equal (no draw change). Log `PM1 bake dl <ptr> nseg nv nidx ok` for each DL seen, cap 40 lines. Host test: NatModel index array equals the `sd` of `dlc_static_build` for the mac_01 DLs (scratch walker output, section 1).
- Test scope: data-only plus shared table. Scenes: census_toad, census_toad_rp.

**P2. Template capture keyed by (DL, segment, render mode, textured).** Files: `3ds/src/pm_gbi.c` (rec_done call, key from group), `3ds/src/pm_native.c`.
- Test: census equal. Log `PM2 tmpl` lines: one per key. Expected count: at most 2 per render mode seen in census_toad.

**P3. Emit one segment per group in one eye (replaces the walk for these groups).** Files: `3ds/src/pm_gbi.c` (mg_run arm for `ok` groups, E1 to E6 eye 0 only), `3ds/src/pm_native.c` (`nat_emit`), delete the walk for these groups in the same commit (rule 7).
- Test: census equal (draw multiset of the emitted segments equals the walker census). Region diff `census_toad` shot against HEAD (0 px expected on the emitted scene if the shot shows the models, with the picture read). Host test for `dep` logic.
- Gate: picture shows the subject (a Toad Town house or prop). Pass only with the picture.

**P4. Texture resolve at emit and sort by template then texture.** Files: `3ds/src/pm_native.c` (bucket pass), `3ds/src/pm_gbi.c` (E3 `tb_run` call).
- Test: census equal. Log count of texture changes per frame (target: one per texture, not per draw). Region diff against P3 build, 0 px.

**P5. Eye 1 in the same walk (stereo).** Files: `3ds/src/pm_native.c` (E4 and E5 for eye 1), `3ds/src/pm_nusys.c` (gbi_task: skip the second walk for `ok` groups, pm_nusys.c:81).
- Test: stereo shot in 3D slider 1 at Toad Town, left and right eye, region diff against HEAD. Seam test (design_native_draw.md section 5): count background pixels inside the union of model silhouettes, must not rise.
- Gate: picture shows both eyes. Test scope: scene with stereo only.

**P6. v2 draw: template words and patch words, no `flush_draw` per segment.** Files: `3ds/src/pm_native.c`, `3ds/src/pm_gpu.c` (one call to write the template block into the command span, no change to flush_draw otherwise).
- Test: census equal, region diff 0 px, byte-equal check on the scenes that use the changed draw path only (shared draw code rule: pm_gpu.c is shared, so compare census_toad and census_yard).

**P7. Delete old path and dead state.** Files: pm_gbi.c (walker arm for the baked groups), no new feature.
- Test: build, grep for no dead references, boot line, one heavy map.

---

## 11. Open items and decisions

- **O1 (decision for the user).** Scope of v1. Option A (recommended): keep the walker for lights (row 2), matrix (row 5) and POPMTX (row 7), and replace the DL, render mode and texture rows. Option B: also replace lights with the `lt[]` snapshot. Option A is smaller and keeps the light path that already passes the tests.
- **O2.** Colour ops in plain groups. Not checked line by line in the ModelRenderModes lists (model.c:190-... and the `Gfx_RM1_*` definitions outside model.c). Check with grep before P2.
- **O3.** Entity and NPC models use other assets. Their shapes may not pass `fix_shape` the same way. Check the load path for entity models before the scope is widened.
- **O4.** Citro3d base address conversion (`BufInfo_Add` to physical, once per buffer). Not in the repo. The design avoids it for the shape (section 0, item 5), but the linear index pool needs the same guarantee. Confirm on the console at P1.
- **O5.** `dlc_on` is on by default. P3 replaces the batch emit for these groups. Confirm with the user that the target is the template emit (section 0, item 2), not a second DLC path.
- **O6.** Budget. NDS1 starts: see agent_nds1.md. This design used none.

Commands for the code agent (from agent_nds1.md): lane build `PM_LANE=nds1 PM_ROW=PMOD sh 3ds/build.sh` after `python 3ds/tools/gen_map_restore.py`, then `git checkout -- 3ds/src/map_restore_table.c`. Census: `azt.ps1 census_toad -Row PMOD -Build nds1 -Try` (justification line first in `3ds/logs/perf/status_PMOD.txt`).
