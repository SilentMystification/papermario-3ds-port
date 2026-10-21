# PT09: emote icon (question mark, exclamation) invisible, the bubble draws (root cause found)

Status: root cause proven by Azahar shots and by code. Fix not applied (pm_gbi.c is main-owned). Bisect against the DR1, TX1 and PG switches: the fault stays, so the switches are cleared.

## Reproduction (Azahar, 2 starts, row INVFX)
- Scene: kkj_14 entry 3, `fast fixedseed autoa warp=kkj_14:3 story=-105`. The scene runs three ShowEmote calls (chapter0.c:102, :202, :353) at game frames 312 to 336 (exclamation), 948 to 964 (question), 1816 to 1848 (exclamation). `fxlist` lists the effect instance in those windows.
- Default switches: 3ds/logs/perf/invfx/s1/shots (one shot every 30 frames). The bubble shows, the icon is missing.
- Switches `nodr1 nomerge notx1 nopg1 nopg3 nodecal2 t1foldall` with `shotwin=312:40`: 3ds/logs/perf/invfx/s2/shots. Same fault. The red "!" shows only in the first frames of the animation (shots 0000 and 0003 of 12 in a row) and is missing in the held frames. Log: 3ds/logs/perf/invfx/s2/log.txt.

## Root cause
- src/effects/emote.c:265 to :303 (emote_appendGfx) moves the window of tile 0 over a sprite sheet with gDPSetTileSize(0, uls * 4, 0, (uls + w) * 4, (ult + h) * 4). Exclamation: sheet D_09000400 96 x 32 (4 bit, mask s 7), w 16, uls = 128 - unk_38 * 16. Question: sheet D_09000A00 128 x 48 (mask s 7), w 32, uls = 128 - unk_38 * 32. Frame table D_E0020CB8 and D_E0020CFC hold unk_38 = 5 and 3 for most frames.
- The RDP reads texel (s - uls) wrapped by the mask (period 128) from the loaded sheet. uls only moves the origin.
- 3ds/src/pm_gbi.c:2530 to :2538 (G_SETTILESIZE, tile 0) binds a texture of the WINDOW size (w = ((lrs - uls) >> 2) + 1 = 17 or 33 texels) from the load origin (column 0 of the sheet). The sheet is not bound. The FX3f block (pm_gbi.c:2532 to :2536) widens the bind to the mask period only for t (tile_wrap[0][3], ld_h), not for s.
- Result: the GPU repeats the first 17 (or 33) columns padded to 32 (or 64). Only a window whose uls is a multiple of the padded width reads the frame 0 picture (uls 128, 96, 64). The held frames (exclamation uls 48, question uls 32) read the padding: alpha 0. Frame 0 of the sheet is also not the held frame. The user sees only the bubble.
- Checked and not the cause: the bubble DL (D_09002170) draws; the icon DL has no culling (G_CULL_BACK cleared); depth (ZB_CLD_SURF, no Z_UPD) is the same for the bubble and the icon.

## Fix text (3ds/src/pm_gbi.c, main-owned)
1. Line 98: `static int ld_valid, ld_x, ld_y, ld_h, ld_w;`.
2. G_LOADTILE (pm_gbi.c:2398 to :2409, next to ld_h), add: `ld_w = ((((int)(w1 >> 12) & 0xfff) - ((int)(w0 >> 12) & 0xfff)) >> 2) + 1;`.
3. G_SETTILESIZE tile 0, right after the FX3f block (line 2536), add the s axis copy of it:
```
if (tile_wrap[0][2] && !(tile_wrap[0][0] & 2) && ld_valid) { /* PT09: sprite sheet, window narrower than the mask period */
    int per = 1 << tile_wrap[0][2];
    if (per > w && per <= 256 && ld_w > w) w = ld_w < per ? ld_w : per;
}
```
The code at line 2538 already sets `stride = timg_stride > w ? timg_stride : w`. tile_x0 stays x0 = uls >> 2. Then u = s - uls reads the sheet and the GPU repeat (padded width 128 for both sheets) gives the RDP wrap.
4. Check the PG1 skip (tsz_last, line 2563): it compares w, h and src. A different w now only changes at the load, not per frame. Keep.
5. The same fault class holds for every effect that slides a tile 0 window over a loaded sheet: grep `gDPSetTileSize` with a changing uls in src/effects (not done).

## Test (1 start)
`fast fixedseed autoa warp=kkj_14:3 story=-105 loglevel=info shots shotwin=312:40`. Pass: the "!" shows in every shot from 312 to 336 and changes shape (6 frames), and the "?" at 948 to 964 shows its 4 frames. Then golden toad and yard. Not tested: the player is Mario here (no PEACH flag), the emote path is the same (EMOTER_PLAYER, npc = PTR_LIST_END).
