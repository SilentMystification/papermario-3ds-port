# agent_tbrep: row TBREP (skip a repeated G_PM_TEX bind list) - result: PASS

## State
- src/pm_gbi.c: tb_run skips a list that is the list it ran last when no key changed. Keys: tex_gen (texture writers),
  pm_tex_epoch(), pm_tex_wrap_gen(), comb_gen, omode_h and omode_l, and sc_ok with sc_sig_now() == sc_sig.
- tex_gen is bumped in tex_op entry, grab_cur, rect_to_image, gbi_texrect, gbi_fillrect, box_pattern_rect,
  pm_texbind_set, and before each pm_tex_load (ensure_tile, dual_bind). rm_apply and gbi_run_body bump
  tex_gen only in the first build, and that made every skip fail, so they do not bump in the final build.
- A list run with a frame buffer texture (pm_is_cfb(timg)) is never recorded.
- sp0c line gets a trailing `tl N` (skipped lists over the 300 frame window). The census DUP0 was never built.
- No toggle, no check switch (tbcheck not built). No new warnings (warning count 3363 at HEAD build, 3363 and 3314 in
  the modified builds, from header and linker lines only).

## Measurements (mac01t_cap1, phase fsplit, sp0c at f483)
| run | lane | sp0c tex (us per frame) | tl (per 300 frames) | ms0 | top shot 0000 | bottom 0001 |
|---|---|---|---|---|---|---|
| HEAD 1 | tbhead | 1799 | - | 69/12/1/33 | baseline | baseline |
| HEAD 2 | tbhead | 1817 | - | - | equal HEAD 1 | equal HEAD 1 |
| mod 1 (rm_apply and gbi_run_body bump) | tbmod | 1850 | 0 | - | equal HEAD 1 | equal HEAD 1 |
| mod 2 (final) | tbmod | 1751 | 5948 (19.8 per frame) | - | equal HEAD 1 | equal HEAD 1 |
| mod 3 (final) | tbmod | 1746 | 5948 (19.8 per frame) | 69/12/1/33 unchanged | equal HEAD 1 | equal HEAD 1 |

- yard (3 of 3 log checks): top shot 0000_0_11 equal between HEAD (start 5) and final build (start 6).
- Census gate in plan_dl_rows.md: tl 19.8 per frame against 20 or more. The count is 0.2 under the gate.
- Azahar numbers are direction only. The console judges speed.

## Risks not checked
- Hits in DL replay (dlc) paths are not traced. A replay that binds a texture without tb_run is outside the keys.
- LOADTILE PG1 path sets tile_x0 and tile_y0 from the last load. A skipped list does not set them again. The shots
  show no difference on mac01t_cap1 and yard. A glyph texture (gox nonzero) is not covered by these two scenes.
- Starts used: 7 of 8 (HEAD mac x2, mod mac x3, HEAD yard, mod yard).

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=TBREP sh 3ds/build.sh`; copy map_restore_table.c
  before the gen and restore it from the copy after the build (never commit it).
- Test: `PM_ROW=TBREP powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row TBREP -Build <lane> -Try`; `yard -Row TBREP -Build <lane>`.
- Quantity: `grep sp0c` in %APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt (the tl field is the last one).

## Open items
- Next row in epic A: EPO1 (plan_dl_rows.md row 4). Its gate uses the DUP0 `ep` count, which still needs the census build.
