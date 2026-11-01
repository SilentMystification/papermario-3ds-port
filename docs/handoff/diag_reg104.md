# diag_reg104: what PICA register 0x104 is, and why yard_rec word j=75 differs

Investigation only. HEAD 5c242e434. No source edit, no Azahar start, no push. Tags: [C] CONFIRMED (code read or file found), [I] INFERRED.

## 0. Correction to the question (CONFIRMED)
- The question says a[75] is the walk value and b[75] is the record value. The code says the reverse.
- pm_gbi.c 3434: `pm_gpu_rec_same(ra + k->off, k->len, w, n, ...)`. The record (ra) is `a`. This frame's words (w) are `b`.
- pm_gpu.c 2310 comment: "a is a valid record, b the words this frame wrote".
- pm_gbi.c 3436 and 3403: `uc2d_first(ra + k->off, k->len, w, n)`. The log prints `a[j]` as the record word and `b[j]` as the frame word.
- So a[75] = 0x00000000 is the RECORD value. b[75] = 0x00000110 is the FRAME value. All claims below use this order.

## 1. What 0x104 is (register name not verified on disk)
- The libctru registers header is NOT on disk. A search of /opt/devkitpro, C:\devkitPro, C:\msys64, deps/ and 3ds/ found no registers.h or citro3d.h. [C]
- The port writes 0x104 through citro3d `C3D_AlphaTest` (pm_gpu.c 490 to 493, sh_alpha_test). [C]
- From memory of the PICA200 register map (3dbrew, libctru naming), 0x104 is the alpha test config (GPUREG_ALPHATEST_CONFIG). Bit 0 = enable, bits 4 to 6 = function, bits 8 to 15 = reference. [I]
- Check on the value 0x110 (under the assumed layout): enable 0, function 1 (ALWAYS), reference 1. [I]
- The port's disabled call is `sh_alpha_test(false, GPU_ALWAYS, 0)` (pm_gpu.c 1014). Under the same layout it encodes 0x010, not 0x110. So 0x110 is not the disabled-call word. [I]

## 2. Who writes it, and the walk and record values
- Source of the state: the display list render mode. pm_gbi.c 651 to 660 set `atest` from the mode bits: 0x08 (CVG_X_ALPHA with blend, or G_AC_THRESHOLD), 0x30 (CVG_X_ALPHA without blend), or -1 (none). [C]
- pm_gbi.c 661 calls `pm_gpu_set_render(..., atest, ...)`. pm_gpu.c 2153 to 2161 stores `r_atest` and calls flush(). [C]
- pm_gpu.c 1013 to 1014 apply it at draw time: `sh_alpha_test(true, GPU_GREATER, r_atest)` or the disabled call. [C]
- sh_alpha_test (pm_gpu.c 490 to 493) returns early when the cache `sh.at` already holds the state. The GPU write only happens on a cache miss. [C]
- pm_gpu_rec_begin (pm_gpu.c 2262) calls `sh_clear()`. So a record captured after a clear always contains the first 0x104 write of the group. The record's 0x104 presence and position depend on the cache state at capture start. [C for the code path; I for the effect on j=75]
- The frame word 0x110 and the record word 0 are both values of the word at j=75 in the group that holds it. The header at the group start is 0x803F0104 (reported by the log). Bit 31 is set in that header. [C for bit 31 check in pm_gpu.c 2300 to 2302 uses `!(hdr >> 31)` as the incremental test; I for the layout]
- Layout of the group: [p0][hdr][extra words]. With count 3 and bit 31 set, words 74 to 77 are data for a non-incremental write. Word 75 may be an extra data word, not the 0x104 write itself. [I]
- Therefore the log line "reg 104" names the group header, not proven to be the register of word j=75. The log does not print the register of word j itself. [C from uc2d_first code, pm_gbi.c 3414 to 3420]

## 3. Does the value depend on the frame?
- No frame input exists in the path. The value comes from the display list mode word `m` (pm_gbi.c 651 to 660) and the blend flags. These are per draw state, not per frame. [C for the code path]
- A texture, light, fog, cull or viewport value is not an input of sh_alpha_test. [C]
- So a record should not patch 0x104 per frame. A mismatch means a different draw state, or a different cache state at capture, for the same record key. [I]
- The record key (RecKey, pm_gbi.c 3352) holds ver, gen, hgen, tgen0, tgen1, s0, s1, state, nb. It does not hold the alpha test state. pm_rec_ver (model.c, not read) may cover it. [C for the field list; I for the gap]

## 4. Single change (proposal, not run)
- Do not mask 0x104. A mask hides a real state difference. [I]
- Make the record key hold the alpha test state of the captured draw. Store `sh.at` (or r_atest) in RecKey at rec_done (pm_gbi.c 3447 area). Reject a hit when the frame's state differs. This is a recapture trigger. [I]
- Expected counter: yard f348 bad 4800 (no mask) or f349 bad 4222 (0x227 mask) falls by the number of 0x104 words that differ. miss stays 0 on yard. The upper bound on the drop is 2375 compares. [I]
- Expected picture: unchanged. The check path does not draw. [I]
- Before the change, one log-only check is needed: print the register of word j itself (walk the group and read the register for word j). This settles the open point in section 2. [I]
- Test: one Azahar start on yard_rec, one justification line in 3ds/logs/perf/status_UC2.txt. Not run here.

## 5. Claims that remain open
- Register name 0x104 and the 0x110 encoding: check against the libctru header when it is on disk. [I]
- Whether pm_rec_ver (model.c) already covers the alpha state. Read model.c. [I]
- Actual UC2D log line: not on disk (no match for "UC2D first" in build3ds or 3ds/logs). The values in the question are unverified. [C]
