# Handoff

Branch `dlc-compiler-c1c2`. HEAD `4d88c3355`. Do not commit `3ds/src/map_restore_table.c`, `README.md`, `docs/handoff/memory/MEMORY.md`, or the deleted `docs/handoff/memory/feedback_workflow.md`. Before a build: `python 3ds/tools/gen_map_restore.py`, then copy `3ds/src/map_restore_table.c.bak` over `3ds/src/map_restore_table.c`. Build with `"C:\Program Files\Git\bin\bash.exe" 3ds/build.sh`. `pm_ticks()` is `svcGetSystemTick` (about 1.3 µs). Do not call it per opcode.

## Speed

O3DS yard, build 1141: wall 53 ms, display-list walk 27 ms, about 19 drawn frames a second. `tile` about 65, `comb` about 60. `build3ds/hw_live_log_170.txt` was empty after that run.

Azahar yard frame 90, build string 1146: `gbisplit sync 429 gv 166 tex 3424` (microseconds). The 3.4 ms is `pm_gpu_flush_at` from `G_SETTILE` (the batch close). `load_inner` in `pm_tex.c` runs later, outside that timer. The combiner rebuild (0.43 ms) and the triangle runs (0.17 ms) are not the 27 ms.

Do not restore these edits. The combiner-flush skip left `comb` at 42. The plan bytes really change (23 layout misses, 19 mode changes). The texture-bind skip (`f971e5dc4` restored it) left the frame at 53 ms and garbled menus. A streak timer of every opcode class ended 1349 times a frame. Do not put that timer back.

Next: fewer batch closes. A yard frame still closes about 65 times on a real texture change.

## Pictures

File-select flip is on the GPU. `draw_box` (`DRAW_FLAG_ROTSCALE`) draws the panel into a VRAM target, then draws strips with a matrix. The start menu and the file select are display lists from VRAM.

On build 1141 the user saw file-select garble, a transparent HUD, title bubbles at about 1/4 of the texture, and a wrong Key Items image. Those were tied to the reverted bind skip. They are not rechecked after the restore.

Repeated letters are still open. `see` draws as `se`. `Boots` draws as `Bo ts`. Site: `pm_gbi.c` `G_SETTILESIZE`, `pm_tex_glyph_same` / `tsz_last`. An untested edit was removed. Do not flip UV signs. Do not add `pm_tex_glyph_ofs` a second time on the new-bind path.

## Log and push

`rsplit`, `combsplit`, and `gbisplit` enqueue to RAM. The writer thread does the file and the socket. `de2dc0966` took `logstat`, the writer-down path, and the sleep flush off the game thread. The debug HUD still `fputs` after `pm_con_trylock`.

Console is 192.168.1.170. Do not touch 192.168.1.131. Do not FTP-pull logs. The user runs Git Bash. Give this command only after an Azahar shot shows the scene:

`PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug`

Last archived binary: `3ds/logs/builds/pm_3ds_1144_log.3dsx`, sha256 `8ac51f4e8ceb401f`, git `de2dc0966`. `build3ds/pm_3ds.3dsx` is build string 1146 and has the texture-command timer. It is not archived. It is not a speed result.

## Tests

One code agent. Model: CLAUDE.md rule 4. No `fast`. New row, budget 3. Do not reset GBI (3/3) or TEXUS (1/3). Yard check: `-Until 'shot 40 frame' -Debug 'loglevel=debug phase shots shotmode=4:80 preset=yard battlef=80'`. The shot must show Mario, the ground, and the shadow.
