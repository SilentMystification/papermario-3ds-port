# BGROW: one 1 px garbled row in the background (diagnosis only)

Symptom (console): in many scenes one horizontal line of the background, 1 px tall, is slightly garbled.
Status: NOT FOUND in the code path. The one Azahar shot did not show the row.

## 1. Background draw path (3DS, sky image 296 x 200 CI8)

- `src/background.c:296`: still background, not wave. On TARGET_3DS the whole sky is one texture bind (`gDPLoadTextureTile` height `h`, `background.c:300-307`).
- `src/background.c:308-309`: `bg_sky_rows(..., row0, rows, t)` for two strips: rows `0..h-y0-1` from source row `y0`, then rows `h-y0..h-1` from source row 0.
- `gBackroundTextureYOffset` (`src/background.c:8`) has no writer outside its init (grep of `src` and `3ds/src`). So `y0 = 0` always. The second strip has `rows = 0` and returns (`background.c:120-122`). There is one strip and no internal strip boundary in this path.
- `background.c:123-124`: rows `yl = (minY+row0)*4`, `yh = (minY+row0+rows-1)*4` (inclusive last row). Matches the COPY mode inclusive rule.
- `src/pm_gbi.c:3229` `height = (yh-yl)/4`, `3235` `height += 1` (COPY inclusive), `3238` `t1 = t0 + dtdy*height/th`, `3244` `ex = xh/4`, `ey = yh/4` (no -1 in COPY). Consistent for integer rows.
- `src/pm_gpu.c:2590-2597`: quad edges `px(x1+1)`, `py(y1+1)`. Texel coordinate lands on pixel centres, so point sampling picks the right texel row. No half-texel error found.
- `src/pm_tex.c:1157` and `1194`: row flip `sy = ph-1-y` (top row stored at the last memory row, matching the comment "PICA samples t = 0 from the last row"). Pad rows (200 to 255) copy row 199 and sit at memory rows 0 to 55, outside the sampled range (56 to 255). No off-by-one found.
- `pm_tex_size` (`src/pm_tex.c:1495`) gives `th = 256` for h = 200 (`pot_at_least_8`, `:280`). Matches the tile.

## 2. Build and wide relevance

- o3dsll build 45fca7dd0: contains 78b4d2db1 (one-bind sky). Does not contain 6c59757fe (wide zoom) or 617540255 (seam). So the one-bind path above is the one that runs on that console when wide is off.
- Wide zoom (6c59757fe, `background.c:85-101`, `bg_zoom` at `:280`): the 6/5 zoom maps rows with integer division. It repeats one source row every 5 screen rows (about 40 repeats), not one line. With `y0 = 0` it adds no strip seam. Mismatch to note: `t1` uses the float height (`pm_gbi.c:3229`) while the pixel count is integer (`pm_gpu.c:2597`). This is a sub-texel slope error (INFERRED, size not measured).
- Seam 617540255 changes `rebuild_3d` (`pm_gpu.c:183-215`): `shader_3d` becomes post x game for the CPU 3D path. Texture rectangles use `tri_raw` with screen-space corners (`pm_gpu.c:2552-2597`), so they do not pass through that matrix. Relation: NONE (INFERRED from code). Docs: `docs/handoff/agent_texgarble.md:29` lists the seam fix for 3D seams only.
- The 6-row wave path (`background.c:311-]`, used on wave stages) is not used by the default still sky. Not the cause for many scenes (INFERRED).

## 3. Azahar check (one start, diagnostic)

- Command: `azt.ps1 yard -Row BGROW -Build hudmap`. Lane hudmap = git 617540255 (contains 6c59757fe and 78b4d2db1). Default (non-wide) run. Log checks 3 of 3 pass.
- Shot: `build3ds/azt/shots/yard/0000_0_11.ppm` (240 x 400 PPM, the 3DS top screen rotated, so a game screen row is a PPM column).
- Sky visible only at PPM x 20 to 219 (game rows 20 to 219). Ground covers most of the sky, so the usable sky band is small (x 140 to 219).
- Column residual test (each column against its two neighbours, sky band, 3 bands of rows): no isolated column above the cloud and tree edges. The only large step is at x 219 to 220 (sky edge to black).
- Exact-equal neighbour test: not usable. Flat 3D colours dominate.
- Result: the 1 px row is NOT seen in this shot. Azahar closed after the run. Justification line: `3ds/logs/perf/status_BGROW.txt`.

## 4. Colour test

Not run. No row was found, so no neighbour comparison exists.

## What would settle it

1. Console shot (or Azahar) of a scene where the sky fills the screen with no 3D ground over it (title or file select), and the row index (screen y) of the garbled line. A row at y 20 (first sky row) or y 219 (last sky row) would match the strip edges in `background.c:123-124`.
2. Whether the console runs with wide on (`pm_opt.wide`, `pm_gpu.c:1222`). Wide decides between the zoom path and the default path.
3. For the flagged y: the colour of that row against rows y-1 and y+1. Equal to y-1 means a repeated source row (texel sampling). A mix means a blend (filter). A colour off the sky palette means a palette or format error (`BG_PAL`, `background.c:13`).
