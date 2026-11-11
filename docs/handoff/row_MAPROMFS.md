# Row MAPROMFS: map change audio hang: the romfs shape read leaves the game thread

(Model: Haiku. Base: main 5d26c84a2. Disjoint from CRASHWALK (pm_main.c) and BTLDBG (pm_warp.c). Reads only this file and CLAUDE.md.)

## User facts
O3DS XL, main f4d7a8061: the map to map transition is laggy; the game audio hangs for a moment at each map change.

## Cause (oracle code read, main 5d26c84a2)
- Audio is step-driven: the game thread kicks the audio ticks once per drawn step (pm_main.c:389-401 pm_audio_step -> pm_audio_kick,
  pm_audio.c:194). A long step leaves the DSP without new ticks: the audio stops until the step ends.
- Path 1 (since HC4f2, 8da4bea37): decode_yay0 (pm_map.c:163) runs on the game thread and, for the shape buffer, reads
  romfs:/shape/<name>.nds there (pm_map.c:190 -> shape_load_romfs :58-94): fopen, an fread of up to 256 KB and the reloc table in
  256-byte freads through the default FILE buffer, all FS service calls on the SD card, inside the map load step.
- Path 2: the same step first fills the shape buffer with the Yay0 decode or the core-1 prefetch copy (:177-182, the prefetch thread
  decodes <map>_shape, pm_prefetch.c:38-47), then the romfs read overwrites all of it: the decode and the memcpy are waste.
- Fix: the core-1 prefetch thread (lowest priority, below audio, pm_prefetch.c:65) reads the .nds file at the start of the map change
  instead of the shape Yay0 decode, with the pointers already relocated to pm_shape_buf; decode_yay0 takes it. The game thread reads the
  file only when no prefetch ran (boot warp), with a 64 KB stdio buffer, and never decodes the shape Yay0.

## Start
`sh 3ds/tools/wt_new.sh MAPROMFS`. PM_ROW=MAPROMFS PM_LANE=MAPROMFS on every build and bench call.

## Touch only
3ds/src/pm_map.c: shape_load_romfs, decode_yay0, one new function after shape_load_romfs. 3ds/src/pm_prefetch.c: pf_maps, pf_start,
pm_prefetch_take, one prototype line. No other file.

## Exact edits
1. pm_map.c shape_load_romfs: add a parameter `u32 base` (the address the relocated pointers point into) and use `v += base;` in place of
   `v += (u32)(unsigned long)buf;`. After `if (!f) return 0;` add `setvbuf(f, NULL, _IOFBF, 0x10000); /* MAPROMFS: a few FS reads, not one per 1 KB */`.
2. pm_map.c after shape_load_romfs add: `int pm_shape_load_nds(unsigned char* buf, unsigned size, const char* name) { return
   shape_load_romfs(buf, size, name, (u32)(unsigned long)pm_shape_buf); } /* MAPROMFS: the prefetch thread loads into a side buffer, relocated for pm_shape_buf */`
3. pm_map.c decode_yay0: replace the block `{ /* MAPX-4: the core-1 prefetch ... */ ... }` (:177-182) by:
   `const int shp = dst == (void*)pm_shape_buf;`
   `if (shp) { extern char wMapShapeName[]; memcpy(pm_shape_name, src == last_asset_src ? last_asset_name : wMapShapeName, sizeof pm_shape_name); }`
   `{ extern int pm_prefetch_take(const void* src, void* dst, unsigned dec, const char* nds); const int pre = pm_prefetch_take(s, dst, dec, shp ? pm_shape_name : NULL);`
   `  if (pre) pm_xf.yay_pre++;`
   `  else if (shp) { if (!shape_load_romfs(dst, dec, pm_shape_name, (u32)(unsigned long)dst)) pm_log_evt("shape load fail %s", pm_shape_name); } /* MAPROMFS: no Yay0 decode of a shape */`
   `  else if (!yay0_decode(s, bound, dst, dec)) return; }`
   In the branch `if (dst == (void*)pm_shape_buf) {` (:188) delete its two first lines (the pm_shape_name memcpy line and the
   shape_load_romfs line); keep the pm_dlc_gen line.
4. pm_prefetch.c: add `int pm_shape_load_nds(unsigned char* buf, unsigned size, const char* name); /* pm_map.c */` after the pm_yay0_raw
   prototype. pf_maps: `s->st = (i == 0 ? pm_shape_load_nds(pf_buf[i], s->dec, s->name) : pm_yay0_raw(s->src, 0x100000u, pf_buf[i], s->dec)) ? 2 : 3; /* MAPROMFS: slot 0 reads the .nds file */`.
   pf_start: the thread stack `8 * 1024` -> `16 * 1024` (stdio and FS calls on this thread now).
   pm_prefetch_take: new last parameter `const char* nds`; in the slot loop first add
   `if ((i == 0) != (nds != NULL) || (nds && strcmp(s->name, nds) != 0)) continue; /* MAPROMFS: slot 0 holds .nds bytes, only a shape takes it */`.
   Fix the comment lines of the file head and of pf_maps that say slot 0 is a Yay0 decode.

## Gate (CLAUDE.md 2; Azahar budget 4 of 8)
- `PM_ROW=MAPROMFS PM_LANE=MAPROMFS sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row MAPROMFS --why "pm_map.c, pm_prefetch.c"`: PASS.
- `bench.py base spot_mac01_det` and `bench.py base xfer_map_shots`; one list `bench.py run spot_mac01_det,xfer_map_shots --row MAPROMFS
  --why "boot shape load on the game thread; mac_02 shape from the prefetch .nds"`; `bench.py cmp` of both: every shot byte-equal.
- The xfer_map_shots log (its LOG: copy) has no `shape load fail` line.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the completing commit. Hand back: the hash, the gate results, the memcheck line. The audio check is the user's
(console push, map to map walk).
