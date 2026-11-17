# ws_BLANKET (Haiku): the toad house blanket list moves to 12 world twins (C5 data -39, C6 data_arrays -1, C7 gfxtype -1)
Designer: oracle NEXT2 (SendMessage). Worktree build3ds/wt/BLANKET, branch BLANKET from main, PM_ROW=BLANKET PM_LANE=BLANKET.
Main at release: 038ac581fe. Shares no file with ENTLST, EFXGT, RECBUF, EFFCEN, DRECTEX, FLTGT. Model: Haiku (exact text, one small tool rule).
## Objective
- FACT (listcen 038ac581fe): assets/us/world/toad_house/blanket.gfx.inc.c data 39, data_arrays 1. ToadHouseBlanketAnim.inc.c:133 includes it.
  12 world files include that .inc.c (list below + dro_02/npc.c). FACT (grep src): no code uses ANS_ToadHouseBlanket or its nodes. No scene draws it.
- FACT (gen_gfx_syms.py:27): IMG matches only a plain name. :134 `INCLUDE_IMG(.., N(toad_house_blanket_img))` gives no symbol. Fix 1 below.
- FACT (gen_gfx_syms.py:141-142): a twin scans its path source. dro_02 twin npc_merlee.c (the only twin with a different first line, FACT loop
  over 238 twins) never scans npc.c, so the blanket vtx and img of dro_02 have no symbol. Fix 2 below.
- FACT (nd_gfx_conv.c:863): the list inherits the cycle bits, so it gets an itab. gset_check (:958) fails the build on a moved list without one.
## Edits (directory order)
1. 3ds/tools/gen_gfx_syms.py. :27 -> `IMG = re.compile(r"INCLUDE_(?:IMG|PAL|RAW)\(\s*\"[^\"]*\"\s*,\s*(?:N\((\w+)\)|(\w+))\s*\)")`.
   After :27 add `IMGNS_RULE = True  # BLANKET: INCLUDE_IMG(.., N(x)) is NAMESPACE_x; seeded test in 3ds/tests/host.d/blanket.sh`
   and `TOPSCAN_RULE = True  # BLANKET: a twin also scans its first-line include when that is not its device source; seeded test in blanket.sh`.
   :108-109 -> `for m in IMG.finditer(text):` / `if m.group(1) and not IMGNS_RULE: continue` /
   `out.append(("Img", name_of(path, ns, m.group(1), ren.get(m.group(2), m.group(2)), pre_bad), moved))`.
   After :142 `scan(src, syms, False)` add: `top = pathlib.Path(m.group(1))` / `if TOPSCAN_RULE and top != src and top.is_file():` /
   `    scan(top, syms, False)  # BLANKET: dro_02/npc.c holds the blanket vtx and img`. Keep :141 byte-equal (gconvw.sh sed matches it).
2. New 3ds/tests/host.d/blanket.sh. Model it on gconvw.sh:34-44 (the GMERLEE block). Work dir /tmp/blanket. Fixture: t/tb.h `#define NAMESPACE tb`;
   t/top.c includes "tb.h", "part.c", "blk.inc.c"; t/blk.inc.c `Vtx N(blk_vtx)[1] = {};` and `INCLUDE_IMG("x.png", N(blk_img));`;
   t/part.c includes "tb.h" and defines `Gfx part_gfx[]`; twin 3ds/tools/gfxin/t/part.c = `#include "t/top.c"`. Output TU gconv_tu_3ds_tools_gfxin_t_part_c.c
   must hold `"part_gfx", GS_GFX`, `"tb_blk_img", GS_IMG` and `"tb_blk_vtx", GS_VTX`. Seeded faults: sed `^IMGNS_RULE = True` -> False (tb_blk_img
   must be absent), sed `^TOPSCAN_RULE = True` -> False (tb_blk_vtx must be absent). cmp each copy with the tool first (not applied = FAIL). Last line `blanket ok`.
3. src/world/common/complete/ToadHouseBlanketAnim.inc.c. After :2 add `#include "nd_gtab.h"`. :4 -> `NDN_DECL(N(toad_house_blanket_gfx));`.
   16 lines: before `    .displayList = N(toad_house_blanket_gfx),` after `    .displayList = NDN(N(toad_house_blanket_gfx)),`. Delete :133.
4. New 3ds/tools/gconv_src.d/BLANKET.txt: line 1 `# BLANKET: twins that hold world/toad_house/blanket.gfx.inc.c`, then these 11 lines:
   src/world/area_{hos/hos_03,jan/jan_03,kkj/kkj_20,kpa/kpa_91,kpa/kpa_95,mac/mac_01,nok/nok_01,sam/sam_02,sam/sam_06}/npc.c,
   src/world/area_kmr/kmr_02/rooms_1.c, src/world/area_kmr/kmr_20/bed.c (one path per line, braces written out).
5. For each path P of step 4: new 3ds/tools/gfxin/P, 2 lines: `#include "P"`, `#include "world/toad_house/blanket.gfx.inc.c"` (form of the kmr_20 records.c
   twin). Append the second line to 3ds/tools/gfxin/src/world/area_dro/dro_02/npc_merlee.c.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-2. `sh 3ds/tests/host.d/blanket.sh` prints `blanket ok` (gconvw.sh needs gcc: docker only, step 4). Commit `BLANKET 1: gen_gfx_syms N() image, twin top scan`.
2. Edits 3-5. Commit `BLANKET 2: toad house blanket list to 12 world twins`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row BLANKET` (base run, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row BLANKET --bench spot_mac01_drec --why "BLANKET gate" > build3ds/out/blanket_loop.txt 2>&1`.
## Tests that decide: `LOOP PASS`: host PASS (holds `blanket ok`, `gconvw ok`), build rc 0 (no `gconv: moved` line), spot_mac01_drec byte-equal. Budget: 2 builds, 2 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: data 39 lower, data_arrays 1 lower, gfxtype 1 lower than on the merge-base (read both).
## Fix rights: only the files above. A twin compile error, a `gconv:` line, a host FAIL or a cmp DIFF: stop, send the output path to the oracle, wait.
## Stop and hand-back
Last commit `BLANKET 3: gate. spot_mac01_drec byte-equal; data <n> data_arrays <n> gfxtype <n>` deletes this page. One SubagentHandback: hashes, LOOP lines, listcen.
