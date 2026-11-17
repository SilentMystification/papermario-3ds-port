# ws_EFFCEN (Haiku): the effcen census drops its list-word count; hud anchor byte count (C2 gfxpos -2, C7 gfxtype -6)
Designer: oracle NEXT1 (SendMessage). Worktree build3ds/wt/EFFCEN, branch EFFCEN from main, PM_ROW=EFFCEN PM_LANE=EFFCEN.
Main at release: 4568e2c157. Edit only src/model.c, 3ds/src/pm_effcen.c, src/hud_element.c. Shares no file with FLTGT, ENTLST, EFXGT,
RECBUF. Model: Haiku (debug tool field removal, one constant).
## Objective
- FACT (nd_rec.c:197-199, :458): each redirected macro takes its packet back, so gMainGfxPos is at nd_rec_top before every record.
  The effcen `ops` value (pm_effcen.c:33, gMainGfxPos - g0) is 0 for every task. It leaves. FACT (grep 3ds/tools): no tool reads effcen lines.
- FACT (hud_element.c:2606, :2614): `slots * sizeof(Gfx)` = ((64 + 7) / 8) * 8 = 64 bytes = `4 * sizeof(Vtx)` (sizeof(Vtx) 16, gbi.h:1092).
- With effcen off (the gate runs), every edit gives the same draw output.
- Counter FACT (listcen on 66cd65f3cd): model.c gfxpos 2 (:4258 :4273), gfxtype 2 (:4078 :4170); pm_effcen.c gfxtype 1 (:30);
  hud_element.c gfxtype 3. Move: gfxpos -2 (12 -> 10 without FLTGT), gfxtype -6.
## Edits
1. src/model.c :4078 `extern void pm_effcen_task(const void* fn, const void* arg, const Gfx* g0, unsigned t0);`
   -> `extern void pm_effcen_task(const void* fn, const void* arg, unsigned t0);`. Delete :4170 `    Gfx* ec_g0 = nullptr;`.
   :4258 and :4273: before `if (ec_on) { ec_g0 = gMainGfxPos; ec_t0 = pm_effcen_t0(); }` after `if (ec_on) ec_t0 = pm_effcen_t0();`.
   :4262 and :4277: `pm_effcen_task(appendGfx, task->appendGfxArg, ec_g0, ec_t0)` -> `pm_effcen_task(appendGfx, task->appendGfxArg, ec_t0)`.
2. 3ds/src/pm_effcen.c: :30 -> `void pm_effcen_task(const void* fn, const void* arg, unsigned t0) {`. Delete :33 (`unsigned words = ...`)
   and :49 (`ec_k[i].words += words;`). :22 drop the field `words`. :41 drop `ec_k[i].words = `.
   :72-73 before `"%08x e%d f%u t%u ops%u us%u | ", ... e->tasks / fr, e->words / fr, (e->ticks ...`
   after `"%08x e%d f%u t%u us%u | ", ... e->tasks / fr, (e->ticks ...`.
   Comment :2-4: drop `the Gfx words its appendGfx emits (gMainGfxPos delta) and` and ` ops<words>`.
3. src/hud_element.c: delete :2606 `    const s32 slots = (4 * sizeof(Vtx) + sizeof(Gfx) - 1) / sizeof(Gfx);`.
   :2614 `nd_rec_data(slots * sizeof(Gfx))` -> `nd_rec_data(4 * sizeof(Vtx))`.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit. `grep -n ec_g0 src/model.c; grep -n words 3ds/src/pm_effcen.c; grep -nw slots src/hud_element.c` prints nothing.
2. Commit `EFFCEN 1: effcen drops the list-word count; hud anchor vertex bytes`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row EFFCEN`, then the same for wscmb_pause_drec (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row EFFCEN --bench spot_mac01_drec,wscmb_pause_drec --why "EFFCEN gate" > build3ds/out/effcen_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main). Budget: 2 builds, 4 Azahar starts.
- listcen: gfxpos 2 lower and gfxtype 6 lower than on the merge-base (read both values).
## What you may edit to fix a fail
Only the 3 files. A compile error or a cmp DIFF: stop. Send build3ds/out/loop_EFFCEN_build.txt or loop_EFFCEN_cmp_<bench>.txt to the oracle.
## Stop and hand-back
Last commit `EFFCEN 2: gate. spot_mac01_drec, wscmb_pause_drec byte-equal; gfxpos <n> gfxtype <n>` deletes this page.
One SubagentHandback: commit hashes, LOOP lines, listcen gfxpos and gfxtype.
