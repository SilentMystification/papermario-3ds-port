# DATAPATH: design study of the path SD -> RAM -> CPU -> GPU (oracle findings; the orchestrator moves this to plan_datapath.md + INDEX.md)

Code read on main of 2026-11-11; ASSETFREE (epic_ASSETFREE.md) stays parked: it moves assets out of the ELF, this study changes formats.

## 1. Data-flow map (format at each hop; per frame = always-on work)
| class | storage | RAM (pool) | conversion, who and when | GPU form | per frame |
| ROM image | SD file | heap, resident (rom_need 25747 KB of 30248 KB heap, boot line #11), streamed at boot (pm_romload.c rs_loader) | none | - | pm_rom_ptr / pi_read_raw memcpy at each asset load |
| map shape (lists, vertices) | romfs shape/<map>.nds (host nd_shape_conv.c: LE, typed ops, relocs) | pm_shape_buf 256 KB (pm_map.c:29) | load: Yay0 decode (wasted, MAPROMFS) + fread + reloc pass | vertices: N64 Vtx 16 B read as is from chunk 0 (dlc/ent compile at first use, linear) | dlc group culling, ranges; no vertex transform |
| map textures | ROM <area>_tex (Yay0) | heap (decoded archive) | load: Yay0; first bind: expand CI4/CI8 to RGBA5551 (CI + dual lut: RGBA8), Morton tile in linear scratch, C3D_TexLoadImage to VRAM slab (pm_tex.c load_miss :1394-1600) | RGBA5551 / RGBA8 / L8 / LA4 tiled, VRAM | per bind: key lookup + palette hash of 16 or 256 entries for CI (pm_tex.c :1724-1740); misses only on new keys |
| sprites | ROM sprite archive (Yay0) | heap (spr_load_sprite decodes all rasters at load, sprite.c:1162) | first bind as textures (CI -> RGBA5551) | VRAM | per bind: palette hash; KZN04 shade: TEV now |
| model records, effects, HUD | built by game code every frame | frame stream (nd_rec.c, core 1) | typed ops at record time (core 1); the walker runs them (core 0) | native draws: command words into the GPUCMD buffer (nd_emit.c), command block copy on a model hit | the whole frame: by design (the game rebuilds its lists each frame) |
| dynamic vertices | game RAM | ring (linear, GRING) | copied per vertex by nd_geom_vtx | ring Vtx | every frame, needed |
| matrices | game Mtx (fixed 16.16) | game RAM | load_mtx: fixed -> float (nd_mtx.c:34), static ones cached by slot/version (nd_mtx_set.c:116-123) | uniforms (palette rows) | moving models only |
| frame copies | PICA colour buffer | VRAM / linear slot | GPU copy (PT12 grab_prev, grab_impl); CPU read fallback (pm_main.c:380) | slot texture | when an effect asks |
| audio | ROM banks | heap + DSP sample cache (pm_dspaud.c) | ADPCM decode to the cache (prefetch predecode for battle songs) | DSP | voice starts |
| scripts, save | .data / SD | heap / SD | none | - | - |
No census today gives resident bytes per class or bytes moved per frame; the numbers above are pool totals from the boot line.

## 2. Target design (RAM holds what the PICA200 reads; the runtime transforms nothing that does not change)
- Textures: host pre-conversion (a converter like nd_shape_conv.c) of map, sprite and effect textures to PICA tiled RGBA5551/RGBA4/
  L8/LA4/ETC1 with the palette applied: removes the expand + Morton + palette hash per bind and the decode at first use. Size: CI4 -> 16
  bit is 4x (a 64x64 CI4 2 KB becomes 8 KB); ETC1 (4 bpp) keeps the size but is lossy and has no alpha (ETC1A4 8 bpp). Blocks: palettes
  the game edits at run time (tattle lerp, palette swaps, sprite palettes per actor): those stay run-time; so the converter keeps CI for
  textures whose palette a script writes. ASSETFREE overlap: the converted pack would come from the user's ROM on first boot.
- Vertices and indices: already native for map shapes; the effect and HUD lists are rebuilt by game code each frame (no pre-convert).
- Matrices: static ones are cached floats; a float side copy at guMtxF2L for moving models would save 16 conversions per matrix (small).
- Command words: model hits copy a prebuilt block (HC6B); a block for entity lists (ent_run) is the next step (PBATCH1 first).

## 3. Shortlist (ranked by expected O3DS gain; rows exist where named)
1. Per-frame texture reloads from scroll windows: TILEWIN (merge), TILEWIN2. 2. Per-particle draws: PBATCH1. 3. Water refraction
source: SPLASHPREV2. 4. Map-change stalls: MAPROMFS, R13NOP. 5. Model cache misses on prim/panner words (kzn_08, hos_01): design read open.
6. Palette hash per CI bind: a palette generation (pm_tex_invalidate covers DMA writes; game-code palette writes do not call it): needs a
   read of every palette writer before a row. 7. Host texture pre-conversion: a design step (size budget against the 14 MB linear and VRAM).
8. ROM image 25.7 MB resident: the largest RAM item; slimming it is the ASSETFREE subject (parked).

## 4. Census rows to add (Haiku, next batch): DPCEN1 bytes per frame (texture bytes uploaded: load_miss gpu_size; source bytes hashed: slen0;
data-cache flush bytes: pm_gpu.c:2943 n and pm_tex.c flush spans; ring vertex bytes; GPUCMD words; frame stream bytes), printed per 300
frames with the switch census; DPCEN2 resident bytes per class by pool (VRAM slab used, texture slots by format, chunk 0 vertex bytes,
mc_pool, ring, frame stream, ROM image, decoded archives) at map load.

## 5. Transform once, ever (user principle; folds into ASSETFREE, investigation only, the epic is not started)
Loop cases today (transform repeats per load or per frame), ranked by O3DS cost; "now" = a row before the epic:
| case | where | repeats | moves to generation | now? |
| seam T-point table of a map shape (16-77 ms per map load: seamfix lines, mac_00 77 ms) | nd_shape.c:42-149 seam_get -> nd_seam_build (nd.h:218, host-buildable: nd_seam.c is in the nd_host build) | every map load | yes: nd_shape_conv.c builds it into the .nds (version 3) | now: SEAMHOST (needs a read of the NdSeamTab layout, nd.h:211-216, for its file form) |
| texture decode CI -> RGBA5551/RGBA8 + Morton tile + upload | pm_tex.c load_miss :1394-1600 | first bind after every map load or eviction | yes: PICA-tiled files per texture (palette applied; CI kept for script-edited palettes) | epic (size) |
| palette hash per CI bind | pm_tex.c :1724-1740 | every bind, every frame | yes for converted textures (no palette at run time) | now: PALGEN needs a read of every palette writer |
| static list compile (dlc groups, boxes, Vtx copies, index pools; ent_compile) | nd_shape.c dlc compile, nd_ent.c :345 ent_compile | first use after every map load | yes: compiled groups and index buffers in the .nds / an entity pack | epic |
| shape reloc pass | pm_map.c:74-88 | every map load | yes: pm_shape_buf is a fixed address per build: pre-applied with the build key | now (small) |
| Yay0 decode of shape (waste), hit, textures, sprites, effect graphics | pm_map.c:163-202, sprite.c:1162 | every load | yes: stored decoded | shape: MAPROMFS; hit: now HITHOST (like shapes); rest epic |
| hit file byte swap | pm_map.c:194 swap_hit_file | every map load | yes | with HITHOST |
| audio VADPCM -> DSP sample cache | pm_dspaud.c predecode | every voice start (cache misses) | yes: DSP-ADPCM or PCM files | epic (SD size) |
| moving matrices fixed -> float | nd_mtx.c:34, load_mtx | every frame | no (game data per frame); a float side copy at guMtxF2L is a separate small row | later |
| model records, effect and HUD lists -> typed ops -> command words | nd_rec.c, nd_walk.c, nd_emit.c | every frame | no: the game builds them per frame (by design); prebuilt blocks for cached draws exist (HC6B) | - |
Native asset files: one per class (shape, hit, texture pack per area, sprite pack, entity pack, audio bank), header { magic, version,
build key = hash of the converter sources + nd_op.h + game build id, payload size, relocs 0 }, payload in the resident form (GPU-ready
for vertices, indices, tiled textures; little-endian structs for the rest). A key mismatch regenerates that class.
Where it runs: host build (today, personal builds only: the ROM-derived data is already in every 3dsx) for shapes (done), seams, hit
files; the release form (ASSETFREE) runs the same converters on the console at first boot from the user's ROM, with a progress screen.
First-boot cost (estimate, to be measured by DPCEN2): SD space roughly 2-4x the decoded asset size for CI textures as 16 bit (ETC1 keeps
4 bpp), time a few minutes on the O3DS (one decode per asset, sequential SD writes), RAM one asset at a time.
Resident form and loader: a direct read into the pool (heap, linear for vertices and indices, VRAM via one upload for textures), no
conversion. What remains of the N64 path: the ROM reader and the decoders only inside the generator; the runtime reads native files.
Next rows (by the oracle after the reads named above): SEAMHOST, HITHOST, PALGEN, reloc pre-apply, DPCEN1, DPCEN2.
