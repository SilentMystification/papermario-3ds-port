# ws_RECBUF (Haiku): entity and sprite record buffers become u32 words (C6 data_arrays -4, C7 gfxtype -15)
Designer: oracle NEXT1 (SendMessage). Worktree build3ds/wt/RECBUF, branch RECBUF from main, PM_ROW=RECBUF PM_LANE=RECBUF.
Main at release: cc8c1817ea. Edit only src/entity_model.c and src/sprite.c. Shares no file with FLTGT, ENTLST, EFXGT. Model: Haiku.
## Objective
- FACT (gbi_custom.h:208, :214): G_PM_SPR_WORDS and G_PM_ENT_WORDS are 4 (4 Gfx = 8 words). FACT (nd_cmd.h:29, :31): nd_rec_spr and
  nd_rec_ent take `const void*`. FACT (nd_gtab.h:135): nd_ntab_find takes `const void*`.
- A record held as `Gfx buf[4]` becomes `u32 buf[8]` with the same bytes. Index rule: `X[k].words.w0` -> `X[2*k]`, `X[k].words.w1` -> `X[2*k+1]`.
  Keep the Gfx alignment: every new u32 array gets `__attribute__((aligned(8)))`.
- FACT (listcen on 66cd65f3cd): entity_model.c gfxtype 8 data_arrays 2, sprite.c gfxtype 7 data_arrays 2. Move: gfxtype -15, data_arrays -4.
## Edits, src/entity_model.c (gfxtype 8)
- :204 `(Gfx*) *curPos++` -> `(void*) *curPos++`. :256 `const Gfx* dl;` -> `const void* dl;`. :258 `ent_vtx_range(const Gfx* dl,` -> `(const void* dl,`.
- :294 `entcheck_draw(const Gfx* rec,` -> `entcheck_draw(const u32* rec,`. :317 `rec[0].words.w0 & 1u` -> `rec[0] & 1u`. :299 `const Gfx* q` -> `const void* q`.
- :341 `Gfx* entRec = nullptr;` -> `u32* entRec = nullptr;`. :342 `Gfx entBuf[G_PM_ENT_WORDS];` -> `u32 entBuf[2 * G_PM_ENT_WORDS] __attribute__((aligned(8)));`.
- :346-353 index rule. Before `        entRec[1].words.w1 = (model->vertexArray != nullptr) ? ...` after `        entRec[3] = (model->vertexArray != nullptr) ? ...`.
- :469 `entRec[1].words.w0, entRec[1].words.w1` -> `entRec[2], entRec[3]`.
- :471 `Gfx endRec[G_PM_ENT_WORDS];` -> `u32 endRec[2 * G_PM_ENT_WORDS] __attribute__((aligned(8)));`. :472-473 index rule.
  :474 -> `endRec[2] = endRec[3] = endRec[4] = endRec[5] = endRec[6] = endRec[7] = 0;`.
- Comments :491 and :770: `entRec[1]` -> `entRec[2], entRec[3]`.
## Edits, src/sprite.c (gfxtype 7; the file includes nd_gtab.h, FACT grep)
- :208 `static Gfx* spr_geo_out;` -> `static u32* spr_geo_out;`. :320-323 index rule (`spr_geo_out[0]` .. `spr_geo_out[3]`).
- :336 `static Gfx* spr_final_list(` -> `static NdLst spr_final_list(`. :389 `Gfx* rmList;` -> `NdLst rmList;`.
- :390 `Gfx* sprRec = nullptr;` -> `u32* sprRec = nullptr;`. :391 `Gfx sprBuf[G_PM_SPR_WORDS];` -> `u32 sprBuf[2 * G_PM_SPR_WORDS] __attribute__((aligned(8)));`.
- :398 `Gfx* rec = sprBuf;` -> `u32* rec = sprBuf;`. :400-409 index rule. Before `        rec[3].words.w1 = (u32)(uintptr_t) spr_final_list(`
  after `        rec[7] = (u32)(uintptr_t) spr_final_list(`.
- :452 `Gfx sprGeo[2];` -> `u32 sprGeo[4] __attribute__((aligned(8)));`.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit both files. `grep -nE "\.words\.|\bGfx\b" src/entity_model.c src/sprite.c` prints nothing.
2. Commit `RECBUF 1: entity and sprite records as u32 words`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row RECBUF`, then the same for entcape_kmr03_drec (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row RECBUF --bench spot_mac01_drec,entcape_kmr03_drec --why "RECBUF gate" > build3ds/out/recbuf_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main). Budget: 2 builds, 4 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: data_arrays 3 (FLTGT not merged) or 1 (FLTGT merged). gfxtype is 15 lower
  than on the merge-base (read both values).
## What you may edit to fix a fail
Only the 2 files. A compile error or a cmp DIFF: stop. Send build3ds/out/loop_RECBUF_build.txt or loop_RECBUF_cmp_<bench>.txt to the oracle.
## Stop and hand-back
Last commit `RECBUF 2: gate. spot_mac01_drec, entcape_kmr03_drec byte-equal; data_arrays <n> gfxtype <n>` deletes this page.
One SubagentHandback: commit hashes, LOOP lines, listcen data_arrays and gfxtype.
