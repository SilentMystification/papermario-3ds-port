# Row S6KEY: the S6T decision key misses inputs that sync_combine reads (main fault: s6chk_yard bad 211)

(SUPERSEDED by ORACLE6: row_S6SYNC.md (Sonnet) continues on branch S6KEY. Try 2 showed that the key was not the cause: the
decision point is. The Haiku coder stops here; the text below is kept as the record of the key words that stay.)
(Model: Haiku. One function in 3ds/src/nd_scomp.c and one constant in 3ds/include/nd_scomp.h. No blocker.)

## Cause (oracle 5 code read, main 3cef2c284)
- s6chk_yard on main (S5EMDL3 base 883aee0e2): `ranges 1955 bad 211`, every bad record the def Gfx_RM1_DECAL_XLU_NO_AA_def
  (0x59c184 in the S5EMDL3 head ELF; the entity SHADOW render-mode list), fields leg_mode, plan_active, ls_use, ls_why: the
  compiled path replays a decision made under other inputs than the op path's sync.
- The op path's combine work is memoized by scm_sig (3ds/src/nd_comb.c:436-441), which keys it by the full omode_h, omode_l,
  `tst1.tile_fmt[0] * 4 + tile_siz[0]` and geom besides the NdCombIn fields: the work (sync_legacy, const_rgb_tex_alpha, decide,
  pm_comb_colours) reads them. The S6T key (nd_scomp_key, nd_scomp.c:72-89) holds only the cycle bits of omode_h (key[19]) and
  none of omode_l, the tile 0 format and size, or geom. A def that does not set them (an RM list sets render mode bits 3-31,
  cycle and geometry, but not omode_l bits 0-2, the TEXTLUT/filter bits of omode_h, or the tile format) finds an entry made
  under another inherited value and replays its decision (nd_scomp_find, :91-105; sc_exec :202-215).

## Start
`sh 3ds/tools/wt_new.sh S6KEY`. PM_ROW=S6KEY PM_LANE=S6KEY on every call.

## Touch only
3ds/include/nd_scomp.h (ND_SC_KEYW), 3ds/src/nd_scomp.c (sc_exec only), 3ds/tools/nd_scomp_test.c (id_of only: line 66).
nd_scomp_key (the pure host part) is not changed. Row S6VORD edits other places of nd_scomp.c and nd_scomp_test.c (unit_class,
nd_scomp_analyse, the s6chk block; the test arrays and checks in main): do not touch them.

## Exact edits
1. nd_scomp.h: `#define ND_SC_KEYW 26u` -> `#define ND_SC_KEYW 30u /* key words: the def, 25 decision inputs, 4 walker inputs of
   the sync memo (sc_exec) */`.
2. nd_scomp.c sc_exec, right after `nd_scomp_key(d, &in, (uint32_t)pm_comb_vprim, key);` (the block has run: nd_ws holds the
   def's writes), add:
   `key[26] = nd_ws.omode_h; key[27] = nd_ws.omode_l; key[28] = nd_ws.tst1.tile_fmt[0] * 4u + nd_ws.tst1.tile_siz[0]; key[29] = nd_ws.geom;`
3. Try 2 (ORACLE6, row fault of try 1: SCOMP TEST FAIL 6). Readers of the key size (grep `ND_SC_KEYW\|nd_scomp_key\|
   nd_scomp_find` in 3ds and src): sc_exec (edit 2), nd_scomp_find (memcmp and hash over all words: no edit), nd_scomp.h's NdSTab
   (no edit), and the host test helper id_of (3ds/tools/nd_scomp_test.c:64-69), which declares `uint32_t k[ND_SC_KEYW];` and
   fills it only through nd_scomp_key (words 0 to 25), so words 26 to 29 hold stack garbage and equal inputs miss. Edit:
   nd_scomp_test.c:66 `uint32_t k[ND_SC_KEYW];` -> `uint32_t k[ND_SC_KEYW] = { 0 }; /* S6KEY: words 26 to 29 are the walker
   inputs sc_exec adds on the device; the host test keys nd_scomp_key's words only */`. nd_gfx_conv.c names nd_scomp_key only in a
   comment (no edit). Expected after it: `SCOMP TEST PASS`.
- Memory: NdSTab grows from 128 x 26 to 128 x 30 words (+2 KB heap, calloc in sc_alloc). memcheck must stay OK.
- S9DEL2 note: these 4 walker inputs join the NDG ctx set (plan_nativeapi.md section 6 item 6): a def keyed at build time must
  also be keyed by them. Write that one line into section 6 item 6 in the same commit.

## Gate (expected values)
- First `git merge main` (main has S5EMDL3 since 3c45cfab6: the s6chk_yard scenario and bench); a conflict: stop and hand back.
- Build, memcheck OK; `bench.py run nd_host --row S6KEY --why "S6KEY nd_host"`: PASS (SCOMP TEST and gtab compare).
- `bench.py base msgbox_talk_det`, `bench.py base spot_mac01_det`; one list `bench.py run msgbox_talk_det,spot_mac01_det,s6chk_yard,
  s6chk_mac01 --row S6KEY --why "S6KEY: full sync inputs in the key"` (4 starts; all normal speed). Bases at the new
  `git merge-base HEAD main`. cmp msgbox_talk_det 1/1 and spot_mac01_det 6/6 byte-equal. s6chk_yard `bad 0` (base 211, all on
  Gfx_RM1_DECAL_XLU_NO_AA_def); s6chk_mac01 `bad 0`. Hand back both `ranges` and `made` values (made goes up: more keys).
- Budget 8 Azahar starts.

## Stop
A byte difference or bad above 0: stop, commit, hand back the s6chk field lines (2 tries per named cause). Delete this row file
in the completing commit.
