# Texture wrap: N64 versus PICA, port choice per case (TEXD3)

Source: 3ds/src/pm_tex.c (line numbers at commit of TEXD3), 3ds/src/pm_gbi.c.
N64 terms: cms/cmt = clamp (bit 1) and mirror (bit 0) per axis, masks/maskt = log2 of the wrap window,
SETTILESIZE window = (w, h) texels of the tile, shifts = shiftS/shiftT.

## Port mechanics (where the choice is made)

| Step | File:line | What it does |
|---|---|---|
| G_SETTILE decode | pm_gbi.c:3019 | cms = bits 8-9, cmt = bits 18-19, masks = bits 4-7, maskt = bits 14-17 |
| wrap choice | pm_tex.c:129-134 `wrap_of` | mask 0 or clamp bit: CLAMP_TO_EDGE; mirror bit: MIRRORED_REPEAT; else REPEAT |
| state set | pm_tex.c:270-279 `pm_tex_set_wrap` | sets the shared `wrap_s`/`wrap_t`, `mk_ws`/`mk_wt` (mask width, 1<<mask), `mk_ms`/`mk_mt` (mirror bits) |
| window fold | pm_tex.c:340-346 (load) and 378-390 (pad) | content of a mask window is copied: `sx = wrap_mod(x, mk_ms ? 2*mw : mw)` with the mirror reflection |
| shift path | pm_tex.c:1450-1457 | a window over 128 K texels is built at mask size, `shr_s`/`shr_t` = 1 |
| bind | pm_tex.c:1616-1618 (`pm_tex_bind`) | ws = shr ? (mirror bit ? MIRRORED : REPEAT) : wrap_s; then C3D_TexSetWrap |
| aux bind | pm_tex.c:1691 | unit 1 uses `aux_ws` from `wrap_of` at `pm_tex_set_aux` |
| wrap callers | pm_gbi.c:3019 (G_SETTILE, tile 0), 1218 (dual_bind, tile 1), 2561 and 2588 (ensure_tile loads) | every call overwrites the shared state |
| cached path | pm_gbi.c:2583 (`tile_bound`) | no `pm_tex_set_wrap` call: the bind uses the state of the last call, which may belong to another tile |

## Case table

| # | Case (N64 tile state) | N64 semantics | Port choice (file:line) | Match |
|---|---|---|---|---|
| 1 | cms 0, cmt 0, mask m > 0, image wider than 2^m | s and t are taken modulo 2^m (no mirror). Texels past the window repeat the window. | REPEAT (pm_tex.c:133). Content built periodic, period 2^m (pm_tex.c:342-345). Pad follows the mask (378-390). | Exact when the texture width is a multiple of 2^m (always, both powers of two). |
| 2 | cms 1 (mirror), mask m > 0, window smaller than the image | s modulo 2^(m+1), reflected in the second half. | MIRRORED_REPEAT (pm_tex.c:132). Content built with `2*mw` period and reflection (pm_tex.c:345). | Exact. The mirror window is symmetric, so GPU reflection of a period-2^m window matches. |
| 3 | cms 1 (mirror), mask = image size or larger | mirror over the mask period. Mask larger than the image: texels past the image come from TMEM (not the image). | MIRRORED_REPEAT (132). `mw` is 0 when the mask is not smaller than the image (345 not used). Pad copies the edge texel (383). | Exact for mask = image size on a power-of-two image. Mask larger than the image: NOT matched (port uses the edge texel, N64 reads TMEM beyond the image). |
| 4 | cms 2 (clamp), any mask | clamp s to the SETTILESIZE window; mask is not applied on a clamped axis. | CLAMP_TO_EDGE (pm_tex.c:131). Pad with the window edge texel (383, `w - 1`). | Exact when mask = 0. NOT matched when mask > 0 and the window is smaller than the image: `mw` (pm_tex.c:336, 345, 378) folds the content by the mask without checking the clamp bit (pm_tex_set_wrap:274-275 sets `mk_ws` from `masks` only). |
| 5 | mask 0 (any cms) | Port comment: mask 0 is clamp on the RDP. | CLAMP_TO_EDGE (pm_tex.c:131). `mw` = 0, no fold. | Port choice, not checked against RDP hardware. Repeat with mask 0 (cms 0) would be a clamp here. |
| 6 | SETTILESIZE window smaller than the loaded image (clamp or wrap) | Coordinates are limited to the window (w, h). | Pad uses `w - 1` and `h - 1` as the edge (pm_tex.c:383 and the row pad). Texture built at the loaded size. | Exact for clamp with mask 0. For wrap, the window fold uses the mask, not the SETTILESIZE window (see case 1). |
| 7 | Window over 128 K texels (shift path) | As case 1 or 2 with the full window. | Image built at mask size, `shr_s` = 1 (pm_tex.c:1457), bind uses REPEAT or MIRRORED by the mirror bit only (pm_tex.c:1616). | Exact for cms 0 and 1. For cms 2 (clamp) the port repeats the mask image instead of clamping. NOT matched. |
| 8 | Stale wrap: the bind uses the state of the last `pm_tex_set_wrap` call | N/A (N64 has no shared state). The bind must use the wrap of the tile that draws. | The bind reads `wrap_s` (pm_tex.c:1616). The last call may be tile 1 (pm_gbi.c:1218 in dual_bind) or a load of tile 1 (2588). The cached path (2583) does not call the setter. | Not proven either way. Session 2 (agent_slot1_texd1.md) forced the tile wrap at every draw: the bind value did not change for 40 binds. |

## Wall 0x0100169c (hos_01, I4 64x64) against the table

- The drawlog chunk #27 (agent_slot1_texd1.md) shows tile cms 0, cmt 0, masks 5, maskt 5. That is case 1. The port would choose REPEAT (pm_tex.c:133). Exact.
- The texword line at frame 200 to 203 shows the bind with wrap s 3 t 3 and shr 0 0 (MIRRORED_REPEAT, from `wrap_s`). That is case 2 or 3 of a tile with an odd cms. The drawing tile of this bind is not the chunk #27 tile. The source of the bound value is not found (stale state, case 8, or another draw).
- If the drawing tile is case 1 and the bind is MIRRORED, the result is not exact for uv outside 0 to 63 texels: the mirror reflects the 32 texel period and the N64 repeat does not. This matches the console garble. Not proven: Azahar showed no measurable change (see below).
- The fix candidate for case 1 with a stale bind is the tile wrap at each bind (case 8). It is not a change of the mapping.

## Azahar result of the TEXD3 switches (default path unchanged)

- Build: the switches `wrapov=repeat|mirror|clamp|edge`, `texnearest=1`, `padov=off` apply only when set. Without them the bind and pad code run with the old values. The td3 log line runs only when a switch is set (first 16 binds).
- Default byte-equal to HEAD: yard 0000_0_11 (start 7) and mac_01 0000_1_2, 0001_1_2 (start 8). Equal to the baselines of starts 1 and 2.
- Wall bind with no switch: texword f200 img 0100169c bind 1, wrap s 3 t 3, filter 1 (start 3). The bind is present.
- Wall bind with `wrapov=repeat`: td3 line `slot 8 programmed ws 2 wt 2 filter 1` (start 5 and 6). The texword line still shows the state wrap 3 (it prints `wrap_s`, not the programmed value).
- Picture: shot at frame 203 (the writer takes 203, not 204). Default against wrapov=repeat: 17355 differing pixels of 96000, bbox 20,52 to 220,325. The same switch run twice (starts 5 and 6): 17755 differing pixels. The run-to-run difference is as large as the switch difference. Azahar does not show a wrap effect on the wall. The mapping change is not measured on the picture.
- The wall surface looks the same in both shots at 240 by 400. The stars differ between runs.

## Open

1. Log the tile index and `tst1.tile_wrap[n]` at each `pm_tex_bind` for slot 8, to see which tile draws 0x0100169c. This is diagnosis: a fresh agent.
2. Case 4 and case 7 (clamp with mask): the port folds by the mask. Check the cases in the scenes that use clamp with a mask window (no test yet).
3. Case 3 (mask larger than the image) and case 5 (mask 0 as clamp): port choice, not checked on the RDP.
4. Coins (tik_03, 0x00571b90, RGBA16 32x32, cms 1, masks 5): the texreg_coin position is not found. Gate not run.
