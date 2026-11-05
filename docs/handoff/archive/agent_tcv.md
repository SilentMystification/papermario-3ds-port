# agent_tcv: row TCV (texture converter check, branch native)

## State
- Commit 7ad64e45f: host test `sh 3ds/tools/tex_conv_test.sh` (TCV1). Files: 3ds/tools/tex_conv_drive.c, tex_conv_test.py, tex_conv_test.sh. Catalog row in docs/handoff/test_catalog.md.
- Commit 09311024f: 4-bit texel parity from the flat nibble index (`tx1_n4` in 3ds/src/pm_tex_tx1.h; `expand` CI4 branch and `nib` and the glyph atlas in 3ds/src/pm_tex.c). Before: odd row stride gave the wrong nibble.
- Test result after 09311024f: 900 requests, 0 failing. Before: CI4, I4 and IA4 failed with odd stride (30 of 90 each). All even strides passed in both states.
- Not committed: azt_scenarios.json (texstride_wall, texdump_coin), texdump_png.py, this file.

## Findings
- Converter (TX1, default path): matches the independent reference for I4, I8, IA4, IA8, IA16, CI4, CI8, RGBA16 (both byte orders), RGBA32, sizes 8 to 64, non-square, masks none, repeat, mirror.
- Wall bind (hos_01, I4 64x64, texture address 010036ac in this build): texword line `stride 64 slen 2048 gpu 64x64 gn 4096 wrap s 3 t 3`. The caller stride is the texel width. The doubled-stride hypothesis (pm_gbi.c 4-bit branch, `stride = r_str * 2`) did not occur on this bind.
- Wall decode: the texsrc bytes decoded as N64 I4 and the texgpu bytes decoded as PICA LA4 agree on all 4096 texels. Both PNGs look the same: mottled grey with horizontal bands. The on-screen wall (shot wall_run1_f204.png, scratchpad) is a mottled blue stone. The ROM is not in the repo, so the source bytes are not checked against the ROM.
- Coin (tik_03, address 00571b90): no bind line for that address in the sewer run (scene frame 2100). No dump. Open.
- Conclusion: the converter is not the wall cause. Remaining candidates: UV or texgen, TEV colour, the mirror wrap (mask 6, cms 1), the source offset of a 4-bit window.

## Open
1. Coin: add a probe that matches RGBA16 32x32 (like `shape` for I4 64x64, pm_tex.c near line 760) or find a coin draw first. Then one start with texdump.
2. pm_gbi.c 4-bit branch (about lines 2966-2971 and 3151-3155): `stride = r_str * 2` and byte offset `r_y * r_str + r_x` for every 4-bit tile with r_v set. Correct only for an 8-bit SetTImg. Not hit by the wall. Check with a diagnosis agent before a change.
3. `sh 3ds/tools/tx1_test.sh` is broken on HEAD: texbake_ref.py extracts TEXD3 code (TEX_SCRATCH_PLANE, td3_parse undefined) and texbake_corpus.py needs PIL. Not fixed.
4. The Python in PATH is the Windows Store alias. The scripts use /c/Python314/python. gcc is /c/msys64/mingw64/bin/gcc (the test script adds it to PATH).

## Commands
- Host test: `sh 3ds/tools/tex_conv_test.sh`
- Wall dump check: `PM_ROW=TCV powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 texstride_wall -Row TCV -Build native -Try`
- Decode a dump pair: `/c/Python314/python -I 3ds/tools/texdump_png.py <sdmc>/texsrc_<addr>_1.bin <sdmc>/texgpu_<addr>_1.bin <out_dir>`, sdmc = %APPDATA%\Azahar\sdmc\3ds\PaperMario.

## Budget
- TCV Azahar starts: 2 of 8 (1 texstride_wall, 2 texdump_coin FAILED: no bind).
