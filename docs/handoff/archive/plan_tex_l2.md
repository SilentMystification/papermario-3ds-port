# plan_tex_l2: TEXL2, a level 2 texture cache in RAM (tiled images of textures that leave VRAM)

Row TEXL2. Plan only. HEAD 029acfcbd, branch uc2c. Line numbers are from 3ds/src/pm_tex.c at HEAD unless a file is named.
CONFIRMED = read in the code or the log at the given line. INFERRED = not measured.
Note: the request names CLAUDE.md rule 10. CLAUDE.md has rules 1 to 9 only. This plan follows rules 5 and 7, and docs/handoff/hw_guidelines.md.

## 0. Result first

- A 4 MB block allocated at boot does not fit the O3DS linear pool beside the measured peaks (section 2.1). The plan uses one linearAlloc for each entry, a 4096 KB cap, and a free floor.
- The CPU must not read VRAM (hw_guidelines.md:9). So a VRAM-to-RAM copy is a GX DMA (GX_TextureCopy), never a memcpy.
- The miss path cost is measured: 61 to 125 ns per output byte on the O3DS console, 750 to 810 us per miss (section 3). The re-miss rate (a miss of a key that was evicted before) is NOT measured. The upper bound of the gain is small on average: 0.05 to 0.08 ms per frame (INFERRED from evicted-after-hit bytes).
- So row L2a measures the re-miss rate first (a key-only ghost table, no data). The rows L2b to L2d start only if L2a passes its gate.

## 1. Today's flow

### 1.1 Lookup and miss
- pm_tex_load (1414) -> load_impl (979) -> load_inner (1294).
- The key is img, tlut, phash (palette FNV, 1347-1368), fmt, siz, w, h, stride, dkey. dkey holds the mask and mirror state (1343), the shaded lut hash (1429), the glyph sheet id (233), or the rgba8 key (1456). CONFIRMED 1374-1375.
- Lookup order: direct index lidx[lk] (1370-1382), the 12 recent slots (1384-1396), then a scan of slots below slot_hi (1397-1409). A hit on a dirty slot hashes the source (slot_valid, 927-958).
- A miss calls load_miss (1007), under PH_TEX (1011).

### 1.2 Convert and stage (load_miss)
1. Size: pot_at_least_8 and the GPU format (1013-1039). gf: CI to RGBA5551 (1016), shaded to RGBA8 (1017).
2. Slot and VRAM budget: free slot, `used + reserve <= tex_cap`, else evict_oldest_done (1073-1088).
3. VRAM extent: slab_alloc (1104), with more evictions while it fails (1105-1109). C3D_TexInit, then tex.data = the slab extent (1135-1149).
4. Convert and tile: TX1 one-pass builds into the second scratch plane (1153-1182). The old expand plus a tile loop (1184-1234) runs only for big or check cases. Scratch is 2 x 1024 KB linear (23, 702).
5. Stage: linearAlloc(gpu_size) (1236), memcpy of the tiled image (1245), pend_add for one merged flush (1247), vram_up = 1 (1249).
6. Key fields, shash = src_hash(img, slen) (1268), stats (1271-1286).
- The tx1 timer covers steps 2 to 6 (tx1_t0 at 1056, sum at 1255). CONFIRMED.

### 1.3 Upload
- pm_tex_upload_vram_before_frame (566) runs before C3D_FrameBegin (pm_gpu.c:1834). It calls upload_vram_slot (541) for each bit in vup_bits: pm_fl_dc of the stage, then C3D_TexLoadImage(stage -> VRAM extent) (545-546), up_frame = tc_frame (549).
- Until the upload, draws of this frame sample the linear stage (tex_for_draw, 627-635).
- C3D_TexLoadImage copies to VRAM with C3D_SyncTextureCopy (a GX TextureCopy plus a wait). INFERRED from the citro3d source. That source is not in this repo.
- pm_tex_begin_frame (813) frees the stage two frames after the upload (828-833). So after 2 frames, the converted image exists only in VRAM.

### 1.4 Eviction and what is dropped
- evict_oldest_done (652-666): the lowest stamp among live slots with epoch != epoch (not drawn in this frame). Counters tc_evict, tc_evict_bytes, and tc_evict_ev0 when uses == 0 (660-663).
- drop (593-618): page filter span removed (596), the stage freed at once with linearFree (598-602), the VRAM extent back to the slab (605-610), used reduced, sgen++ (614), live = 0.
- So an evicted texture loses its VRAM image and its stage. The next use is a full miss: ROM-resident source read, convert, tile, stage, upload.
- Other drops: stale slots at the frame start (826), bank B drain (783, 786-799), invalidation with changed bytes (slot_valid 956, tex_invalidate_body 916).

### 1.5 Log counters (texstat or phase on, tc_report 738-774, every 300 frames)
- `texstat f<N>: hits, miss, evict (KB), never hit before evict (KB), invalidated, uploaded KB` (762).
- `texstat live`, `texstat age`, `texstat largest 0..2: WxH fmt siz KB uses age` (764-769).
- `tx1 f<N> miss <n> KB <k> us <t> ns/B <x>` (770): misses, KB, us and ns per output byte of the miss path (steps 2 to 6).
- `texmap since last map: hit miss up inval same diff evict inv_us hash_us` (pm_mem.c:240), with switch texmax.
- phase lines carry `tex` ms, `texmiss`, `texKB` per frame.

## 2. Design

### 2.1 Where the pool lives, and the memory numbers
The pool must be linear FCRAM. The GX DMA reads it for the upload, and the GPU samples it while a stage is drawn (tex_for_draw).

O3DS (CONFIRMED, console logs):
- Commit cap 64 MB (pm_mem.c:18, 85). Linear pool 14208 KB, heap 30844 KB (hw_live_log_131.txt:17). Fixed need 4968 KB (:20).
- Linear used at the first map 7084 KB, free 7124 KB (hw_live_log_131.txt:251).
- Linear peak on the console: 8580 KB (ftp_170b/hw_o3ds_log.txt:1844). Peak in the Azahar 6-map tour: 10648 KB (build3ds/texinv_run6.txt:9169).
- Heap: free-in-arena 11 KB, sbrk-left 4388 KB (hw_live_log_131.txt:251). The 4 MB sbrk reserve is the N64 game heap (pm_mem.c:10). The heap cannot give 4 MB.
- A fixed 4096 KB block leaves 14208 - 8580 - 4096 = 1532 KB at the console peak, and -536 KB at the Azahar tour peak. It does not fit.

N3DS: no number. hw_live_log_224.txt has 0 lines. The build caps the commit at 64 MB on both consoles (pm_mem.c:18, 85; MODE line pm_main.c:631). INFERRED: the N3DS has the same 14208 KB pool and the same fit. A larger N3DS pool needs a cap change in __system_allocateHeaps. That is a separate row, not in this plan.

Decision:
- One linearAlloc for each entry (the pattern that the stage already uses, 1236). Cap L2_CAP = 4096 KB.
- Floor: a put is refused when linearSpaceFree() < L2_FLOOR = 2048 KB. linearSpaceFree costs 2 to 5 us (ftp_170b/hw_o3ds_log.txt:121, :1523). One call for each put only.
- When the stage linearAlloc fails (1236-1243), free the oldest L2 entry and try again, before the texture fails.
- So L2 yields its memory to the fixed users. On the O3DS the real size can be less than 4 MB.
- Risk: fragmentation of the linear heap (docs/plans/topology.md:223). Check: `texture failures 0` and linear free min in the mem map lines. Upgrade path, only if the check fails: one boot block with an extent table like slab_alloc (510-535).

### 2.2 Data structure
- `#define L2_N 512` (equal to SLOTS). Direct-mapped: bucket = l2key & (L2_N - 1).
- l2key = the lk hash (1370) XOR shash. The content hash is in the bucket key, so two contents at one address (map 8 and map 9 textures in one TextureHeap address) use different buckets.
- SoA hot arrays: `u32 l2k[512]` (2 KB, 0 = empty), `u32 l2_stamp[512]` (2 KB). Cold record: `L2Ent l2e[512]`, 64 bytes each, aligned 32 (2 lines): buf, img, tlut, phash, shash, dkey, slen, tlen, gpu_size, reserve, w, h, stride (u16), pw, ph (u16), fmt, siz, gf (u8), ready_frame.
- A lookup reads one l2k word and one record. No scan. The cap eviction scans l2_stamp only (2 KB, at a put that goes over the cap). Fits rule 2 of hw_guidelines.md.
- Order: LRU by the slot stamp at the put (the same stamp counter as evict_oldest_done). A bucket collision frees the old entry (lossy, like lidx).
- Bytes per entry: 64 B record + the image (reserve, 512 B minimum, slab_size 497).

### 2.3 Put (on eviction from VRAM)
In evict_oldest_done (652), before drop:
1. Skip if s->native, s->img == NULL, or s->dirty. Skip in bank B while bdrain (that half can be freed in the same begin_frame, 836).
2. If s->vram_stage is still live (the upload was less than 2 frames ago): move the pointer to the entry, set s->vram_stage = NULL so drop does not free it. Cost: 0 bytes copied. ready_frame = up_frame + 2.
3. Else (row L2d) and s->uses > 0: buf = linearAlloc(gpu_size), pm_fl_dc(buf, gpu_size) so no dirty line is written back over the DMA result, add {VRAM extent, buf, gpu_size} to a readback list. ready_frame = tc_frame + 2.
   - Skip uses == 0. 44 percent of evicted bytes were never hit (docs/SLICES_DONE.md:260). Console: 2664 of 6117 KB (ftp_170b), 3117 of 6464 KB (ftp_170c).
4. pm_tex_upload_vram_before_frame (566) issues the readback list first, then the uploads. Call shape: `GX_TextureCopy(src, 0, dst, 0, n, 8)` (the plain copy that pm_gpu.c:433 uses with dims). The freed extent can go to a new texture in the same frame, but that texture's upload runs after the readback in the same function. So the readback reads the old image. INFERRED: the GX queue runs in order.

Cost of the copy:
- memcpy from VRAM: not allowed (hw_guidelines.md:9).
- GX DMA: no CPU work per byte. CPU cost = one flush of the destination plus one GX command. The DMA time on the console is NOT measured. Row L2a adds a timer around pm_tex_upload_vram_before_frame (us per op), which is the same kind of GX copy.
- Reconversion: 61 to 125 ns per output byte on the console (section 3). A 4 KB slot = about 300 us. A 256 KB background = about 19 ms (ftp_170b/hw_o3ds_log.txt:1787, tex 20.8 ms with texKB 272).
- INFERRED: the DMA is cheaper than the reconversion for each byte. The DMA runs for every put, but a hit happens only on a re-miss. L2a gives both numbers before L2d.

### 2.4 Lookup (on a VRAM miss)
In load_miss, after gpu_size and gf are known (1039), before the conversion:
1. h = src_hash(img, slen). The miss already computes this hash at 1268. Compute it once here and use it at 1268. No extra cost.
2. Read bucket (lk ^ h). Hit when all key fields are equal, shash == h, gf, pw, ph equal, and tc_frame >= ready_frame.
3. On a hit: skip the conversion (1040-1069, 1153-1234), skip the linearAlloc and the memcpy (1236, 1245). Set s->vram_stage = entry buf, free the bucket (the entry leaves L2: the cache is exclusive). The normal upload runs before the next frame. The stage is freed by the existing rule (828).
4. Slot alloc, VRAM extent and C3D_TexInit stay as they are (1073-1152).
- The uploaded bytes are equal to the bytes of the first conversion. The key is the slot key plus the content hash. So the picture is byte-equal. CONFIRMED by the key at 1374-1375.

### 2.5 Invalidation
- Source change: the hash is in the key and is compared at each hit. No hook in pm_tex_invalidate is needed, so its hot path (6946 calls in one step, 416) does not change. Same method as tex_inv2 (slot_valid 927-958).
- Palette change: phash in the key (1348-1368).
- Mask, mirror, shaded lut, glyph, rgba8: in dkey or width (1333-1346, 1429, 233, 1456).
- Frame buffer sources: never put (s->native, 1270).
- Map change: no flush. Entries of the old map age out by LRU. An old entry whose address now holds other bytes never matches (hash).
- Handle reuse and versions: a hit makes a new slot through the miss tail. sgen of that slot index was bumped at its drop (614). The RK2 and DLC records compare sgen, so they re-resolve. CONFIRMED.
- Hash collision: 32-bit FNV, about 2^-32 per check (review_tex_cache.md finding 7). Same risk as today.

### 2.6 Threads
- All pm_tex calls run on the game thread (core 0). pm_os.c:394 skips pm_tex_invalidate on the audio thread. pm_gpu.c:1834 and 1844 call the upload and begin_frame on the game thread. The log sink threads (pm_log.c:819) only write text. CONFIRMED.
- L2 needs no lock.

### 2.7 Staging buffers and the garble suspect (docs/handoff/agent_texgarble.md:17, :68)
The suspect: upload_vram_slot queues a DMA, and drop() frees the stage at once (599).
L2 rules, so L2 adds no early free:
- A buffer is in one owner only: the slot (as vram_stage) or one L2 entry.
- L2 frees a buffer only when tc_frame >= ready_frame (2 frames after the last upload or readback). The cap eviction and a bucket collision skip entries that are not ready.
- A buffer that is a readback target is not given to a lookup before ready_frame.
- After a hit, the existing +2 rule (828) frees the buffer.
- The evict path moves the stage to L2 instead of the linearFree in drop. So for evictions the early free goes away.
Order with TEXGARBLE: start L2b after the TEXGARBLE fix is committed. If that fix moves the stage free in drop() to a deferred release, L2 takes the buffer at that same release point. Do not write a second free path.

### 2.8 Stats line
In tc_report, after the tx1 line (770), with pm_log_hw:
`texl2 f<N> hit <n> miss <n> put <n> evict <n> skip <n> KB <live KB> rb <n> rbKB <k> us <hit path us>`
- hit and miss: lookups in load_miss. put: entries added. evict: entries freed by cap, floor or collision. skip: puts refused (floor, uses 0, native, bdrain). KB: live L2 bytes. rb: readbacks. us: time of the hit path (compare with the tx1 us per miss).

## 3. Expected gain

Measured miss cost (O3DS console, CONFIRMED):
| Log | Frames | Misses | KB | tx1 us | ns/B | us per miss | Evicted | Evicted KB | Never hit KB |
|---|---|---|---|---|---|---|---|---|---|
| build3ds/ftp_170b/hw_o3ds_log.txt (14 texstat windows) | 4200 | 970 | 10418 | 785363 | 74 | 810 | 524 | 6117 | 2664 |
| build3ds/ftp_170c/hw_o3ds_log.txt (16 windows) | 4800 | 1762 | 16986 | 1326276 | 76 | 753 | 525 | 6464 | 3117 |
| build3ds/lane-texmax/last_log.txt (Azahar, 420-map tour, 101 windows) | 30300 | 15414 | 91461 | 5764686 | 62 | 374 | 8739 | 59406 | 21398 |

- Average miss cost: 0.19 ms per frame (170b), 0.28 ms per frame (170c).
- Spikes: ftp_170b/hw_o3ds_log.txt:1787-1789, frames 3538 to 3540, `tex` 18.3 to 20.8 ms with texKB 257 to 272. The texture is 296x200 CI8 (256 KB) with uses 0 (:1794-1795). It is a first load, so L2 does not help it. INFERRED.
- Eviction windows: ftp_170b:1791 (f3600, 474 evictions, 5662 KB, 2383 KB never hit), ftp_170c:1241 (f1200, 149 evictions, 1092 KB, 47 KB never hit), ftp_170c:3175 (f5100, 141, 3720 KB, 2738 KB never hit).
- Possible re-miss: ftp_170c, area 1 map 1 loads at :1202 (evictions at f1200) and again at :2728. The window after it (f3600, :2959) has 53 misses, 257 KB, 22 ms. Some of these can be L2 hits. INFERRED.
- Map 8 and map 9 alternate at ftp_170c:2187-2547. The windows f2700 to f3300 have 77 to 133 misses each with 0 evictions and 394 to 752 invalidated. These misses come from invalidation, not eviction. An L2 put from the hash-change drop (row L2e) can make them hits. INFERRED.

Upper bound for the eviction case (every evicted-after-hit byte comes back while it is in L2):
- 170b: (6117 - 2664) KB x 74 ns/B = 261 ms in 4200 frames = 0.062 ms per frame.
- 170c: (6464 - 3117) KB x 76 ns/B = 260 ms in 4800 frames = 0.054 ms per frame.
- Azahar tour: (59406 - 21398) KB x 62 ns/B = 2.4 s in 30300 frames = 0.08 ms per frame. A one-way tour seldom comes back, so the real value is lower.
- The 4 MB pool holds about 1.3 x the 3072 KB VRAM budget (22). With LRU, a re-miss is a hit if less than 4096 KB of other victims were put after it. The number of re-misses inside that window is NOT measured. Row L2a measures it.
- A hit saves the conversion, the linearAlloc and the memcpy. It keeps the hash, which the miss also does. INFERRED: 80 percent or more of the tx1 us of that miss.

## 4. Rows

Each row: at most 60 changed lines, in 3ds/src/pm_tex.c only. Affected scenarios: yard and volcano (kzn_19) only. Rule 5: pm_tex.c is shared draw code, so the byte-equal check runs on those 2 scenes.
New scenario (row L2a, added to 3ds/tools/azt_scenarios.json and the test_catalog.md table in the same commit): `volcano_l2` = volcano plus `texstat loglevel=debug texcap=1024`, until-line `texl2 f300` (L2a: `texghost f300`). texcap forces evictions in one map: tex_cap is checked at 1076.
Baselines: yard and volcano against HEAD before L2a. volcano_l2 against the L2a binary (HEAD has no texcap).

### L2a: measure first (about 40 lines). Go or no-go for L2b to L2d.
- Debug switch `texcap=<KB>`: after slab_init, tex_cap = min(tex_cap, KB x 1024). 3 lines. A test tool, it may stay.
- Ghost table: 1024 buckets x 8 B (key, cumulative put bytes), direct-mapped by (lk ^ shash). Put at evict_oldest_done (cause E) and at the slot_valid hash-change drop (cause D). On a miss, a match whose put is less than 4096 KB old counts as a ghost hit. Counters per cause: hits and KB.
- Compute src_hash once at the start of load_miss (move from 1268).
- Timer: us and count in pm_tex_upload_vram_before_frame (per frame, per op).
- Line: `texghost f<N> evict-hit <n>/<KB> diff-hit <n>/<KB> upload ops <n> us <t>`.
- Gate (Azahar): volcano_l2, texghost evict-hit above 0. Pictures of yard and volcano byte-equal to HEAD (no pixel change).
- Console (the user): push with `texstat phase`, walk town, a map change and back, one battle. Read texghost KB x 74 ns/B. Go for L2b only if the re-miss saving is 0.2 ms per frame or more on average, or 2 ms or more in one spike window. Otherwise stop here and keep only the counters.

### L2b: table and put by stage move (about 50 lines)
- Arrays and record (2.2), put step 2 (2.3) only, cap and floor and collision free (ready rule 2.7), free at stage alloc failure, texl2 line (2.8).
- Gate (Azahar, volcano_l2): texl2 put above 0, KB at or below 4096, `texture failures 0`, yard and volcano byte-equal to HEAD.

### L2c: hit path (about 40 lines)
- Lookup and hit (2.4). Exclusive move of the buffer to vram_stage.
- Gate (Azahar, volcano_l2 with `texcap=256` for fast re-misses): texl2 hit above 0, texl2 us per hit below tx1 us per miss of the same run, volcano_l2 byte-equal to the L2b binary, yard and volcano byte-equal to HEAD.

### L2d: readback put (about 45 lines)
- Put step 3 and 4 (2.3): readback list, pm_fl_dc of the target, GX_TextureCopy before the uploads, ready_frame.
- Gate (Azahar, volcano_l2): texl2 rb above 0, texl2 hit above the L2c run, pictures byte-equal as in L2c. Azahar does not model GX timing. The DMA cost is a console number.

### L2e: put from a hash-change drop (about 15 lines, only if the L2a diff-hit KB is large)
- slot_valid drop (956) and its stale case (947-954) put with the same readback. Exclude bank B drain.
- Gate: the ftp_170c map 8 and map 9 case. A scenario that warps between 2 maps that share a TextureHeap address (new scenario, catalog row). texl2 hit above 0, pictures byte-equal.

## 5. Console measurement (after the epic, the user)
Push: `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug`. Live log build3ds/hw_live_log_170.txt.
Read:
1. texl2: hit, miss, put, evict, KB, rb, rbKB, us. us per hit against tx1 us per miss.
2. tx1 us per 300 frames against ftp_170b and ftp_170c (section 3 table), on the same route.
3. phase `tex` ms on the map-revisit frames.
4. texghost upload us per op (the DMA cost).
5. `mem map linear used`, linear free min, `texture failures 0`.
6. By eye: no wrong texture after a map change and back, and in a battle.
