# Row HCCUT5: nd_tev_build rare arms cold (class E/H, byte-equal). Starts after HCCUT2 merges (same file nd_tmpl.c).

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read, main a3458b866; M3CEN: nd_tev_build 1988 B runs 8-10 calls per frame, top of E and H)
- nd_tev_build (3ds/src/nd_tmpl.c:136-272): after the combine arms, two rare tails:
  (a) the off-screen 8-bit image arms (:229-253): `int memb = off_active && off_i8 && k->r_memblend && !r_replace; if (memb && env_hi
  <= 4) { ... } else memb = 0; if (off_active && off_i8 && !(r_blend && !r_replace) && !memb && env_hi < 6) { ... }` (FX2a, X02);
  both need off_active && off_i8 (an off-screen pass into an 8-bit image).
  (b) the depth view (:255-267): `if (k->dview && k->draw_3d && !k->rect_2d) { ... }` (V02, a debug view).
- HCCUT2 edits nd_tmpl_build, build_words, tmpl_index, tmpl_victim in this file: start from main after the HCCUT2 merge.
## Start
`sh 3ds/tools/wt_new.sh HCCUT5` from main after HCCUT2 is merged; PM_ROW=HCCUT5 PM_LANE=HCCUT5.
## Touch only
3ds/src/nd_tmpl.c: nd_tev_build (:229-271) and two new functions above it. No other line.
## Exact edits (same statements in the same order)
1. New `static __attribute__((cold, noinline)) int tev_off_i8(NdTevOut* o, const NdTmplKey* k, int* env_hi)` holding the two blocks
   of (a) with `env_hi` replaced by `*env_hi` and the locals they read taken from k (off_active = k->off_active, off_i8 = k->off_i8,
   r_replace = k->r_replace, r_blend = k->r_blend); it returns memb. In nd_tev_build the lines of (a) become
   `int memb = 0; if (off_active && off_i8) memb = tev_off_i8(o, k, &env_hi); /* HCCUT5: off-screen 8-bit pass */`.
   (Check: with off_active && off_i8 false, the old code gives memb 0 and runs neither block: equal.)
2. New `static __attribute__((cold, noinline)) void tev_dview(NdTevOut* o, uint32_t* env, int tex, int* env_hi)` holding the body of
   (b) (`env_hi = 6` -> `*env_hi = 6`); in nd_tev_build `if (k->dview && k->draw_3d && !k->rect_2d) tev_dview(o, env, tex, &env_hi);`.
   `env` is the local stage pointer nd_tev_build already uses in (b); keep its type as declared there.
## Gate (CLAUDE.md 2)
- build: memcheck OK. `bench.py run hotset_size --row HCCUT5 --lane HCCUT5 --why "nd_tev_build cut"`: rc 0, M1hot E and H lower.
- `bench.py run nd_host --row HCCUT5 --why "HCCUT5"`: PASS (nd_host builds nd_tmpl.c: the template tests run).
- `bench.py base` mdl5_iwa10_det and spot_mac01_det; one `run mdl5_iwa10_det,spot_mac01_det`; `cmp` each: 6/6.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
