# ws_IMGOVL (Haiku): imgfx overlay list gets its captured entry (C10 recdl -1)
Designer: oracle DESIGN3. Worktree build3ds/wt/IMGOVL, branch IMGOVL from main AFTER the IMGFXT merge. PM_ROW=IMGOVL PM_LANE=IMGOVL.
Edit only src/imgfx.c (the one line below and its desc line); add 3ds/tools/azt.d/IMGOVL.json, 3ds/tools/bench.d/IMGOVL.json,
3ds/tools/fxrest.d/cap5.txt.
## Facts (code read, main c599b1527c; IMGFXT edits imgfx.c: match the text, not the number)
- imgfx.c:1805 `    nd_rec_dl(ufs->displayList);` is the last recdl site outside src/effects. It is already the plain capture form
  (entapply.py:32). The line before it logs `imgfx strip f<n>` once when the path runs (imgfx.c:1804).
- Overlay users: monstar battle (src/battle/area/sam2/actor/monstar.c:177) and sam_05 (src/world/area_sam/sam_05/npc.c:101, :104).
  axns_sam2_drec (azt.d/AXNS.json) runs the monstar battle with drec at 400-440.
- The combine before the call has 2 forms (imgfx.c:1785-1791: OVERLAY_RGBA or not). GUESS: monstar uses one form, so one tuple.
## Steps (each output to build3ds/out/, read the last 20 lines)
1. IMGOVL.json (azt.d and bench.d, copies of axns_sam2_drec and its bench), 2 scenarios and 2 benches, cap 30, deadline 30:
   imgovl_cap: the axns_sam2_drec debug line with `drec=400:1 entcap entcapt psochk` in place of `drec=400:3:20`, until
   `drec f400 draws [1-9]\d* bytes \d+`, events add `imgfx strip f\d+`.
   imgovl_drec: the axns_sam2_drec debug line plus ` psochk`, until `drec f440 draws [1-9]\d* bytes \d+`.
   Commit `IMGOVL 1: imgovl_cap, imgovl_drec`.
2. `sh 3ds/build.sh > build3ds/out/imgovl_build1.txt 2>&1`. `python 3ds/tools/bench.py run imgovl_cap --row IMGOVL --why "IMGOVL
   capture"`. `python 3ds/tools/entcap.py <its run.log> > build3ds/out/imgovl_entcap.txt`.
   `grep "src/imgfx.c:" build3ds/out/imgovl_entcap.txt | grep -v " so " > 3ds/tools/fxrest.d/cap5.txt` and keep only the lines
   of the nd_rec_dl line number. cap5.txt must hold the line(s) of that site only.
3. `python 3ds/tools/entapply.py 3ds/tools/fxrest.d/cap5.txt > build3ds/out/imgovl_apply.txt`. Expected: `entapply site
   src/imgfx.c:<n> pe_imgfx_<n>` and `entapply sites 1 ...`. Commit `IMGOVL 2: imgfx overlay captured entry; cap5.txt`.
4. `python 3ds/tools/bench.py base imgovl_drec --row IMGOVL --commit $(git merge-base HEAD main)` (base run, not charged).
5. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row IMGOVL --bench imgovl_drec --why "IMGOVL gate" > build3ds/out/imgovl_loop.txt 2>&1`.
## Tests that decide (2 builds, 3 Azahar starts)
- `LOOP PASS`: host PASS, build rc 0, imgovl_drec byte-equal. psochk end line: bad equal to the base run, draws higher.
- listcen total recdl 1 lower than on `$(git merge-base HEAD main)`. lkey: `python 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt .`
  prints `lkey census ok`.
## Fix rights: only the 4 files. No `imgfx strip` line, a `multi` line (send the 2 word sets), a DIFF or a higher bad: stop,
  send the output path to DESIGN3 and wait.
## Stop: last commit `IMGOVL 3: gate. imgovl_drec byte-equal, psochk bad equal; recdl <n>` deletes this page. One SubagentHandback.
