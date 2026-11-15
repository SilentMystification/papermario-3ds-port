# ws_DLCRD (Haiku): read the .dlc file whole, buffer the .seam read (map change lag)
Objective: USER FACT 2026-11-15: map changes lag on the O3DS XL. FACT (build3ds/hw_o3ds_log.txt, build 5774): `seamfix map mac_00_shape ... kb 11
us 151700 src file` (151.7 ms for 11 KB), `dlcpre f450 pre 64` and `f451 pre 63` (127 lists read from the .dlc in 2 frames).
FACT code: dlcp_load (3ds/src/nd_shape.c:269) does one fseek and 2+ freads on the romfs FILE per list. Each fseek drops the stdio buffer, so
each list costs FS reads. seam_load (nd_shape.c:158) has no setvbuf (pm_map.c:67 has one). GUESS: each FS read costs about 1-10 ms on the O3DS.
Counter: none (perf). Worktree build3ds/wt/DLCRD, branch DLCRD. Edit only 3ds/src/nd_shape.c.
## Edit 1: a buffer for the whole .dlc
Before (line 202): `static FILE* dlcp_f;`  After: `static FILE* dlcp_f;` and a new line `static u8* dlcp_buf; /* DLCRD: the whole .dlc; dlcp_f reads it (fmemopen) */`
## Edit 2: dlcp_close (lines 208-212)
Before: `    if (dlcp_f) fclose(dlcp_f);` / `    free(dlcp_idx);` / `    dlcp_f = NULL; dlcp_idx = NULL; dlcp_n = 0; dlcp_gen = 0;`
After:  `    if (dlcp_f) fclose(dlcp_f);` / `    free(dlcp_idx); free(dlcp_buf);` / `    dlcp_f = NULL; dlcp_idx = NULL; dlcp_buf = NULL; dlcp_n = 0; dlcp_gen = 0;`
## Edit 3: dlcp_open
Before (lines 228-229): `    f = fopen(path, "rb");` / `    if (!f) return 0;`  After: the same 2 lines, then
`    setvbuf(f, NULL, _IOFBF, 0x10000); /* DLCRD: a few FS reads (pm_map.c:67) */`
Before (line 244, inside `if (dlcp_n)`): `        free(b);`
After:
```c
        const u32 end = seam_le32(b + 8u * dlcp_n + 4u); /* DLCRD: the end offset of the last section */
        free(b);
        dlcp_buf = (u8*)malloc(end ? end : 1u);
        if (!dlcp_buf || fseek(f, 0, SEEK_SET) != 0 || fread(dlcp_buf, 1, end, f) != end) goto bad;
        fclose(f);
        f = fmemopen(dlcp_buf, end ? end : 1u, "rb"); /* the list reads below stay fseek and fread, now in memory */
        if (!f) { free(dlcp_buf); free(dlcp_idx); dlcp_buf = NULL; dlcp_idx = NULL; dlcp_n = 0; return 0; }
```
Before (lines 248-251): `bad:` / `    fclose(f);` / `    dlcp_n = 0;` / `    return 0;`
After:  `bad:` / `    fclose(f);` / `    free(dlcp_buf); free(dlcp_idx); dlcp_buf = NULL; dlcp_idx = NULL;` / `    dlcp_n = 0;` / `    return 0;`
## Edit 4: seam_load
Before (lines 158-159): `    f = fopen(path, "rb");` / `    if (!f) return -1;` (the .seam open, the second fopen in seam_load)
After: the same 2 lines, then `    setvbuf(f, NULL, _IOFBF, 0x10000); /* DLCRD: one FS read for the whole .seam (max 13536 B) */`
Do not change dlcp_load, the index format or any other reader. A compile error on fmemopen: add `#include <stdio.h>` at the top (mechanical fix).
## Loop and gate (1 build, 2 Azahar runs)
1. Edits 1-4. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/dlcrd_host.txt 2>&1`. Pass: PASS. Commit.
2. One build. Run `bench.py run spot_mac01_drec --row DLCRD` and `bench.py run sbk14_drec --row DLCRD`. Then
   `bench.py cmp <bench> --row DLCRD --commit $(git merge-base HEAD main)` for each. Pass: 1/1 byte-equal each, and the row log of
   spot_mac01_drec has a `dlcpre` line with `pre` above 0 (the file path ran; FACT console log: `dlcpre f35 pre 113`).
3. A differing drec or no `pre` above 0: stop. Write the output to build3ds/out/ and send the path to the oracle.
Stop: both gates pass. Commit `DLCRD 2: gate` with the values, delete this page in that commit.
Hand-back: build3ds/out/handback_DLCRD.txt (commit, host result, 2 cmp results, the dlcpre line). Then the user sends the console line
`xfer2 map n<k> | dlc <n>/<ms> ... seam <ms> dlcp <ms> entt <ms>` of one mac_01 to mac_00 change, before and after this row.
