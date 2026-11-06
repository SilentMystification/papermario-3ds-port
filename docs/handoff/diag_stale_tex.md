# Diagnosis: stale texture on native draws (paths, fences show Mario walk frames)

Scope: read-only trace of the native texture bind path at b09ac893c. No build, no Azahar run. Hypotheses not yet checked against dro_01 logs.

## 1. Every way a cached or native draw can bind a slot with changed or replaced texels

A. Native lookup matches by content hash only, and never runs slot_valid.
- pm_tex.c:1693-1703 pm_tex_slot_find matches shash + fmt + siz + dim. It ignores img address, tlut address, phash (Slot.phash, pm_tex.c:30), dkey, w, h, stride, and the dirty flag.
- nd_tex.c:96-115 nd_tex_unit_hint: the hint branch checks key (pm_tex_slot_key = shash, pm_tex.c:1680) and gen only; the fallback branch calls pm_tex_slot_find. Neither calls slot_valid (pm_tex.c:1079).
- Walker lookup (pm_tex.c:1539, 1553, 1564) compares img, tlut, phash, fmt, siz, w, h, stride, dkey and calls slot_valid, which rehashes a dirty slot (pm_tex.c:1082-1110).
- Result: a dirty slot (tex_inv2, pm_tex.c:1035-1041: source overwritten, slot kept, shash old) is returned to native draws with its old texels. A palette-different copy with equal source bytes is returned too.

B. Descriptor memo never re-reads source.
- nd_tex.c:63-72 nd_tex_get: a memo hit returns the entry with e->key (shash captured once at load, nd_tex.c:85) and e->w. No check that the bytes at d.img still hash to e->key. A buffer rewritten at the same address keeps the old key.

C. Generation does not protect dirty slots.
- sgen is bumped at drop (pm_tex.c:696), at tex_inv2 invalidation (pm_tex.c:1047, 1064) and at stale_mark. Gen changes make the hint miss, but the find fallback (nd_tex.c:113) then returns the same dirty slot, since its shash is still the old key.
- Slot reuse itself is covered: every reuse goes through drop, which bumps sgen (pm_tex.c:673-701). The epoch-stale path (pm_tex.c:1059-1066, 1100-1106) keeps live=1 and bumps sgen.

D. "Most recent equal copy".
- pm_tex.c:1697-1700 picks the highest stamp among equal copies, dirty ones included. The walker binds the copy it looked up by address. Native and walker can pick different copies.

E. Unit words without force, shadow skips equal words.
- nd_tex.c:102, 114 and pm_tex.c:1687 set u->force = 0 for native unit words. The walker sets force on sgen or nd_lt change (pm_tex.c:1668-1669). Native never does.
- nd_emit.c:74 skips the unit block when addr, dim, param, lod and type equal the shadow. The texture-cache clear (nd_emit.c:81) only runs when a block was written.
- A slab freed and reused at the same address with the same dim/param/lod/type is therefore not rewritten and not cache-cleared, unless the context dirty bit is set (pm_gpu.c:939). drop() sets it through C3D_TexBind(0, NULL) (pm_tex.c:691). That covers the drop case if citro3d sets the bit for NULL; not verified (no citro3d source read).
- ndn_sh is reset only when ndn_mode is 0 (pm_gpu.c:938) or the dirty mask is set (pm_gpu.c:939).

F. Hit path reuses hinted unit without a content check.
- pm_gpu.c:2861 uses the hinted slot; pm_gpu.c:2871 stores u into q->d.u[0] only when e->tex. Unit 1 is never set by model draws (nd_model.c:234 sets u[0] only). No bug there.

G. Sprite path.
- pm_gpu.c:3153-3200 pm_gpu_spr_build uses ds.img = record raster (r.raster, nd_sprite.c:10) and calls nd_tex_get + nd_tex_unit (same lookup as A and B). pm_gpu.c:3204 pm_gpu_spr_submit writes the unit.
- A sprite raster that is a per-frame DMA buffer at a fixed address shares a memo entry with any descriptor at that address (nd_tex.c:63-72 memcmp includes img).

## 2. Most likely cause

A (with B). Native draws bind by content hash without the walker's validity rules.

Story: a buffer at address M is used as a texture source by a path/fence descriptor and is rewritten each frame by Mario's walk-frame DMA (invalidate, so the slot goes dirty, pm_tex.c:1035-1041). The walker rehashes at its next lookup (slot_valid) and reloads. The native hint and find (nd_tex.c:96-115, pm_tex.c:1693) never call slot_valid, so the dirty slot keeps its old texels. Depending on which frame was in M at first load, a path or fence draws a Mario walk frame until the slot is evicted. The "sometimes" fits eviction timing. Mario's own sprite draws are less visible because they also keep one stale frame.

Less likely: D (copy choice) and E (texture cache with equal words).

## 3. Debug check and fix

Debug check (no change to behaviour, log only, first 20 hits, behind a debug switch):
- At pm_gpu.c:2861 (pm_gpu_model_hit) and in nd_tex_unit_hint success (nd_tex.c:102, 114): compare
  - slot identity: pm_tex_slot_img(hs) (new getter, returns s->img) vs e->d.img;
  - slot dirty flag (new getter) and pm_tex_slot_tlut vs e->d.pal;
  - content: src_hash(e->d.img, slen) vs e->key (the descriptor's bytes now vs the bytes the key was taken from).
- Log the descriptor address, format, dims and the three values. A line with dirty=1 or hash != key on a path draw confirms A/B.
- Same check at pm_gpu_spr_build (pm_gpu.c:3170) for the sprite raster.

Fix (about 25 lines, in pm_tex.c and nd_tex.c):
1. pm_tex.c: add `int pm_tex_slot_clean(int i, const void* img, const void* tlut)`: returns 1 when slot i is live, not dirty, s->img == img, s->tlut == tlut (the walker rule, pm_tex.c:1539).
2. pm_tex.c:1697 pm_tex_slot_find: skip slots with dirty set or whose img/tlut do not match the descriptor (pass the descriptor img and pal in, or do the check in nd_tex.c).
3. nd_tex.c:96 nd_tex_unit_hint: the hint branch requires pm_tex_slot_clean(*hs, e->d.img, e->d.pal). On failure, fall through to pm_tex_load(d->img ...) (already present at nd_tex.c:112-113) and re-find with the new key. Also set e->key from the new load (pm_tex_slot_key) so the memo follows the bytes.
4. nd_tex.c:63 nd_tex_get: on a memo hit, if the entry's slot is dirty (pm_tex_slot_clean false), reload through pm_tex_load and refresh e->w and e->key.
5. Secondary (E): in pm_tex_slot_unit (pm_tex.c:1685-1688) set u->force = 1 when the slot's sgen differs from the sgen last written to unit 0 by native (keep a static nd_u0_sgen in the native submit). This removes the reliance on the citro3d dirty bit.

Build and test: per CLAUDE.md, debug check first; no Azahar run from this agent. The fix row needs a dro_01 scenario with the path and fence draws, and a picture that shows the path texture.

Not verified: citro3d C3D_TexBind(0, NULL) dirty bit, dro_01 logs, which descriptors in dro_01 share an address with a Mario raster.

## 4. Result of step 1 and step 2 (TEXSTALE, lane tex)

Step 1 (commit 0046c8b6e), dro_01 walk, 300 frames, switch texstale: binds 25509, bad 1322, dirty 3, img 0, pal 1319, hash 0. The mismatches are palette pointers only: the slot holds the same image address and the same source hash as the descriptor, but a different TLUT address (the native lookup ignores the palette). No native bind drew stale texels in this window (hash 0, dirty 3 with unchanged hash). Hypothesis A (dirty slot bound by native) is not confirmed for dro_01 in 300 frames. The walk-frame symptom has not been reproduced yet.

Step 2 (commit c04b00e93): native lookup takes only a clean slot (pm_tex_slot_clean: live, not dirty, same img and tlut). A miss or dirty copy reloads through pm_tex_load and takes the key of the loaded bytes. Unit force is set when the slot or its generation changed. Gates on lane tex: dro_01 walk texstale binds 25513 bad 0; spot_mac01_ndmcheck ndm_diff 0, eq 13727. The dro_01 shot (0002, frame 150 to 155) was viewed: Mario and partners, a wall, no clear path or fence picture in view. A person must confirm the subject.

Open: the walk-frame symptom from the user report is not yet tied to a bind. Next check needs a longer dro_01 window (or the frames in which Mario's walk DMA rewrites a bound source) with texstale on, to see whether bad ever shows dirty or hash mismatches.
