# Row SCOMP0: state compiler: PRIM, ENV and texture units as ordered value units; no def id cap

(Model: Haiku. Exact code below. nd_scomp_analyse, unit_class and the host test only. SCOMP1 edits other functions of the same
file in parallel. No caller changes, so the picture cannot change.)

## Why (oracle 4 code read, main eacd96ed9)
- Entity gaps (S6E) and the sprite def (S5SPR4) hold PRIM, ENV and texture units (TIMG, TILE, TILESIZE, LOADBLOCK, LOADTILE,
  LOADTLUT). unit_class (3ds/src/nd_scomp.c:25) refuses them. They become value units (run as ops, in order). Rule: such a unit
  must not lie between two static units (it would move after a static unit that follows it: texture setters read othermode TT
  bits, PRIM and ENV setters sync with the combine state), else the def is refused. Their setters keep sc_sig exact: every
  decision input they change is in sc_sig_now (nd_comb.c:468-474), so the next draw syncs again.
- `def_next >= 16` refuses (:58). The id is a hint only (`last[def_id & 15u]`, :86); key word 0 is the def pointer. Remove the cap;
  the counter becomes atomic (core 0 analyses entity defs, core 1 the producer defs).

## Start
`sh 3ds/tools/wt_new.sh SCOMP0`; PM_ROW=SCOMP0 PM_LANE=SCOMP0 on every build and bench call.

## Touch only
3ds/src/nd_scomp.c: line :14, unit_class (:16-27), nd_scomp_analyse (:29-61). 3ds/tools/nd_scomp_test.c. Nothing else.

## Exact edits
1. :14 `static uint8_t def_next;` -> `static unsigned def_next;`.
2. unit_class: before `default:` add `case NDO_PRIM: case NDO_ENV: case NDO_TIMG: case NDO_TILE: case NDO_TILESIZE:
   case NDO_LOADBLOCK: case NDO_LOADTILE: case NDO_LOADTLUT: return 4u; /* SCOMP0: an ordered value unit */`. The comment of
   `default:` becomes `/* RARE, draws and control */`; the class comment (:16) adds `4 ordered value`.
3. nd_scomp_analyse:
   - after `int ok = ..., seen = 0;` add `unsigned first = 32u, last = 0u; uint32_t om = 0;`;
   - after `if (c == 0u) { ok = 0; continue; }` add `if (c == 4u) om |= 1u << k;` and change `if (c == 2u) {` to
     `if (c == 2u || c == 4u) {`;
   - after `seen = 1;` add `if (first == 32u) first = k;` and `last = k;`;
   - replace the two lines `if (ok && def_next >= 16u) ok = 0;` and `if (ok) d->id = def_next++;` with
     `for (unsigned k = 0; k < 32u; k++) if ((om >> k & 1u) && k > first && k < last) ok = 0; /* between two static units */`
     `if (ok) d->id = (uint8_t)__atomic_fetch_add(&def_next, 1u, __ATOMIC_RELAXED); /* a hint only (last[]) */`.
4. nd_scomp_test.c: bad_u's second unit -> `NDO_W0(NDO_VTX, 0), 0,`, its check text -> "a def with a vertex load is refused".
   Add three static unit arrays and checks after it (same 21-zero NdSDef initializer as :55-57):
   - pre_u: `NDO_W0(NDO_PRIM, 0), 0x11223344u, NDO_W0(NDO_OMH, OM_A(G_MDSFT_CYCLETYPE, 2)), G_CYC_1CYCLE` -> check
     "prim before the first static compiles" (analyse 1, nv 1, pre 1);
   - mid_u: the OMH unit, the PRIM unit, `NDO_W0(NDO_OML, OM_A(0, 2)), 0` -> check "prim between static units is refused"
     (analyse 0);
   - tex_u: `NDO_W0(NDO_COMB, 0xfffe7e), 0xfffffbfdu, NDO_W0(NDO_TIMG, 0), 0, NDO_W0(NDO_TILE, 0), 0` -> check "texture units
     after the last static compile" (analyse 1, nv 2, pre 0).

## Gate (expected values)
- `PM_ROW=SCOMP0 PM_LANE=SCOMP0 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row SCOMP0 --why "SCOMP0 ordered value units"`: PASS; `SCOMP TEST PASS`, 0 FAIL.
- `PM_ROW=SCOMP0 PM_LANE=SCOMP0 python 3ds/tools/bench.py base spot_mac01_det`; then
  `python 3ds/tools/bench.py run spot_mac01_det,s6chk_mac01 --row SCOMP0 --why "SCOMP0: no caller change"` (2 starts);
  `bench.py cmp spot_mac01_det --row SCOMP0`: every shot byte-equal; the s6chk line has `bad 0`. Budget 4 starts.

## Stop
A FAIL line, a byte difference, `bad` above 0, an edit that does not fit, or a test-tool fault: commit, hand back the output.

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate values, the memcheck line.
