# Row PALGEN: the palette hash of a CI bind is computed once per palette per frame (memo), not at every bind

(Model: Haiku. Base: main. Edits 3ds/src/pm_tex.c load_inner, the phash block (:1734-1755) only. Disjoint from DPCEN2 (other pm_tex.c
lines: none), TILEWIN2, FILLA0, PBATCH2. Reads this file and CLAUDE.md.)

## Facts (oracle code read, main)
- Every CI texture bind (load_inner, the hit path too) hashes its palette: 16 or 256 u16 entries, FNV over u32 pairs
  (pm_tex.c:1734-1755), so the texture key follows in-place palette edits.
- Palette writers in RAM: the texture archive decode (pm_tex_invalidate covers DMA and decode writes), the sprite and HUD palette swaps
  (they change the pointer, not the bytes), and game code that edits palette bytes (tattle lerp, imgfx palette ops, background blends).
  Game code writes run on the logic thread between the walker's frames or during the walk of the previous frame (core 1 runs step N+1
  while core 0 walks frame N), so the bytes a bind sees can change between frames, not between two binds of one walked frame (one frame's
  binds run in one walk on core 0; a write during that walk is the existing race, equal for a memo and for a rehash).
- Rule: the first bind of a palette (pointer, entry count) in a walked frame hashes it; later binds of that palette in the same frame
  reuse the value. A 64-entry direct-mapped memo keyed by pointer and count, tagged with the walker's frame number.

## Start
`sh 3ds/tools/wt_new.sh PALGEN`. PM_ROW=PALGEN PM_LANE=PALGEN on every build and bench call.

## Exact edit (3ds/src/pm_tex.c load_inner)
Wrap the hash computation: before `unsigned phash = 0;` add
`static struct { const void* p; int n; unsigned f, h; } ph_memo[64]; /* PALGEN: palette hash per walked frame */`
and replace `if (fmt == FMT_CI && tlut) { phash = 2166136261u; { ... } }` by
`if (fmt == FMT_CI && tlut) { const unsigned pf_ = pm_frame_count(); const unsigned ps_ = ((unsigned)(uintptr_t)tlut >> 4) & 63u;`
`    if (ph_memo[ps_].p == tlut && ph_memo[ps_].n == tlut_n && ph_memo[ps_].f == pf_) phash = ph_memo[ps_].h;`
`    else { phash = 2166136261u; { <the existing hash block, unchanged> } ph_memo[ps_].p = tlut; ph_memo[ps_].n = tlut_n; ph_memo[ps_].f = pf_; ph_memo[ps_].h = phash; } }`
(pm_frame_count is declared in pm_port.h.)

## Gate (CLAUDE.md 2; Azahar budget 5 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PALGEN --why "palette hash memo"`: PASS; hotset_size PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,dpcen_mac01 --row PALGEN
  --why "same keys; fewer palette hashes"`; `bench.py cmp` of both maps: 6/6 byte-equal each (pass gate); the dpcen line's texmiss is not higher
  than DPCEN1's mac_01 value (13 per 300 frames).

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
