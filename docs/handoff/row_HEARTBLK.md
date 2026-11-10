# Row HEARTBLK: status bar digits draw from the glyph cell texture (one rect batch per number)

(Model: Haiku. New worktree: `sh 3ds/tools/wt_new.sh HEARTBLK` from main 019d6ec8c. PM_ROW=PM_LANE=HEARTBLK on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Commit after each passed step. Delete this row file in the completing commit.)

## Goal
The HP/FP count-up lag (path 1 of the old row): each status bar digit is its own 16x16 CI4 raster (ui_status_text_0_png .. _9_png, src/global_hud_scripts.c:354-381, HES_StatusDigitN 16x16 at :1978-1996), so each digit binds its own texture and its own rect batch. The glyph cells (3ds/src/pm_tex.c:186-360, MSGBOX 3) already put CI4 glyphs of any palette into one 512x512 texture. The walker keeps the batch open when pm_tex_glyph_same returns 2 (nd_tex_set.c:216, :636; the palette load does not flush when the cell is bound, :555). Those paths are generic: only glyph_resolve decides. So the fix is in pm_tex.c only: glyph_resolve also knows the 10 digit rasters.
## Cause (code)
glyph_resolve (pm_tex.c:216) accepts only the three message sheets (glyph_sheets, :201). cell_write (:274) reads `g->base + idx * raster`.
## Touch only
3ds/src/pm_tex.c: glyph_resolve, cell_write, new table hud_digit_img and hud_digit_sheet. Not nd_tex_set.c (busy, COINSTAR). Not the times (8x8) and slash (8x16) icons.
## Exact edits (pm_tex.c, after glyph_sheets[])
1. Add:
```
/* HEARTBLK: the status bar digits (16x16 CI4 rasters, HES_StatusDigit0-9) bind a glyph cell, so the digits of a number are one rect batch. */
extern unsigned char ui_status_text_0_png[], ui_status_text_1_png[], ui_status_text_2_png[], ui_status_text_3_png[], ui_status_text_4_png[],
    ui_status_text_5_png[], ui_status_text_6_png[], ui_status_text_7_png[], ui_status_text_8_png[], ui_status_text_9_png[];
static const u8* const hud_digit_img[10] = { ui_status_text_0_png, ui_status_text_1_png, ui_status_text_2_png, ui_status_text_3_png, ui_status_text_4_png,
    ui_status_text_5_png, ui_status_text_6_png, ui_status_text_7_png, ui_status_text_8_png, ui_status_text_9_png };
static const GlyphSheet hud_digit_sheet = { NULL, 0u, 16, 16, 128, 4u };
```
2. glyph_resolve: after the sheet loop, before `return 0;`:
```
    if (w == 16 && h == 16)
        for (i = 0; i < 10u; i++)
            if (img == hud_digit_img[i]) { *sh = &hud_digit_sheet; *idx = (int)i; return 1; }
```
3. cell_write: `const u8* src = g->base ? g->base + (size_t)idx * (size_t)g->raster : hud_digit_img[idx];`
## Premise (code read only, no run)
In src/hud_element.c the status bar digit draw loads the raster with gDPLoadTLUT_pal16 plus a CI 4-bit tile of 16x16 and point filter (inventory.c:392-465 clear FILTER_TEX). load_impl (pm_tex.c:1359) calls cell_load only with `!lut && dkey == 0`. If the code shows another size, a lut or a dkey for these loads: stop and hand back the file:line.
## Done criteria
1. Build OK. memcheck OK. nd_host PASS.
2. `bench.py base yard_hud_pic`, `bench.py run yard_hud_pic --row HEARTBLK --why "digit cells"`, `bench.py cmp yard_hud_pic --row HEARTBLK`: shots byte-equal to base. The log line `glyph cells f<n> used <u>` has u >= 3 (base: 0).
3. The same three commands for yard_wide_hud_pic (1.25x bar): byte-equal.
Stop: a byte difference: hand back with the cmp output. After the merge, the orchestrator does one XL prof push (kmr_03, hp=1, jump under the block at 130 60 0).
