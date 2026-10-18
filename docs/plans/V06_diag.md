# V06 diagnosis (read only, 2026-10-18)

## Cause
The port draws the old background picture in a building room because the texture cache does not see a changed palette.

## Evidence
- Decomp, `src/world/script_api/rooms.c` RoomVisibilityToggleImpl: the room fade calls `mdl_set_shroud_tint_params(0,0,0,alpha)` up to alpha 255.
- Decomp, `src/background.c` appendGfx_background_texture: tint mode NONE or SHROUD reads that alpha (a1) as fogA. It writes `gBackgroundPalette[i]` every frame. At fogA 255 all 256 entries are `BG_PAL(1)` (black). Otherwise the entries move toward black.
- The background raster is CI8. It is drawn in 6-line strips with G_LOADTLUT of `gBackgroundPalette` (one fixed address).
- Port, `3ds/src/pm_tex.c` load_impl line 367: a cache hit needs only `img`, `tlut` pointer, fmt, size and stride to match. It does not compare the palette content. The first strip load keeps the bright palette. Later frames hit the cache, so the picture never turns dark.
- Port, `pm_tex_invalidate` runs only on decode (pm_map.c:375) and file read (pm_os.c:317), not on a C write to the palette.
- Fits HISTORY.md 1349: "the background picture stays". The ground fade works because V06 added the blend mix, so only the background is wrong.
- Not run on Azahar (F03 edits graphics code, no build).

## Fix plan
- File: `3ds/src/pm_tex.c`.
- Add `unsigned phash;` to struct Slot (line 18 to 22).
- In load_impl, when `fmt == FMT_CI && tlut`, compute an FNV-1a hash over `tlut_n * 2` bytes of `tlut` (max 512 bytes) before the cache loop. Else phash = 0.
- Add `&& s->phash == phash` to the hit test at line 367. Store `s->phash = phash` where `s->tlut = tlut` is set (line 465).
- A changed palette now misses, converts a new texture and frees the old one by the normal LRU. Cost: one 512 byte hash per CI load, and a re-convert only on frames where the fade changes the palette (about 8 frames per door).
- Do not invalidate by pointer each frame. That would re-convert the background every frame in all maps.
- Callers: grep shows `pm_tex_load` calls in pm_gbi.c lines 832, 1244, 1829 only. All pass tlut and tlut_n, so no caller change.
- Other effect: any other CI texture whose palette is edited in place now also updates (correct).

## Test command
`pwsh 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Warp "mac_00:..."` is not needed. Use the Russ T door preset from HISTORY.md 1348:
`pwsh 3ds/run_azahar.ps1 -Preset toad -PadScript "wait80,right*38,up*60,up+A*4,up*20,wait60" -Turbo -Smoke -FaultStop`
Take one shot after the door. Expect black outside the room. Then run `python 3ds/tools/golden.py yard toad` for unchanged scenes.
Check kmr_20 and kmr_02 doors with one `tour=` run.
