# ws_GTIN: build.sh GT_IN covers the own-TU sources of the converter set (tool fault fix, no C counter)
Model: Haiku. PM_ROW, PM_LANE = GTIN. Branch GTIN, worktree build3ds/wt/GTIN. Budget: 1 build, 0 Azahar runs. Start now (main 40518d695).
Owns: 3ds/build.sh (one new line after :97) and new 3ds/tests/host.d/gtin.sh. Edit no other file.
FACT (build.sh:97-98): the stamp key `want` hashes only the GT_IN paths. GT_IN names src/effects and src/entity, not the other .c sources of 3ds/tools/gconv_src.d/*.txt
(FACT GCONVW.txt: 32 sources, for example src/imgfx.c, src/pause/pause_gfx.c). An edit to one of them alone does not rebuild 3ds/src/nd_gfx_tab.c.
## Group 1 (1 build, commit `GTIN: ...`)
1. New file 3ds/tests/host.d/gtin.sh (LF), exact text:
```
#!/bin/sh
# GTIN: build.sh GT_IN covers every .c source of 3ds/tools/gconv_src.d (an edit to one rebuilds nd_gfx_tab.c). Seeded fault: the first GT_IN line alone must miss a source.
srcs=$(sed 's/#.*//' 3ds/tools/gconv_src.d/*.txt | tr -s ' \t\r' '\n' | grep '\.c$' | sort -u)
cover() { for p in $srcs; do ok=0; for w in $1; do case "$p" in "$w"|"$w"/*) ok=1; break;; esac; done; [ $ok = 1 ] || echo "$p"; done; }
GT_IN=; eval "$(grep '^    GT_IN=' 3ds/build.sh)"
miss=$(cover "$GT_IN"); [ -z "$miss" ] || { echo "gtin FAIL: not in GT_IN: $miss"; exit 1; }
GT_IN=; eval "$(grep -m1 '^    GT_IN=' 3ds/build.sh)"
[ -n "$(cover "$GT_IN")" ] || { echo "gtin seeded fault not seen"; exit 1; }
echo "gtin ok"
```
2. Seeded check before the build.sh edit: `sh 3ds/tests/host.d/gtin.sh` must print `gtin FAIL: not in GT_IN:` and a source list. Else stop and send the output to the oracle.
3. build.sh: after :97 (the line that starts `    GT_IN="3ds/tools/gen_gfx_tab.sh`) add one line:
   `    GT_IN="$GT_IN $(sed 's/#.*//' 3ds/tools/gconv_src.d/*.txt | tr -s ' \t\r' '\n' | grep '\.c$' | sort -u | tr '\n' ' ')" # GTIN: the own-TU sources of the set`
4. `sh 3ds/tests/host.d/gtin.sh` prints `gtin ok`. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gtin_h1.txt 2>&1`: `ND HOST TESTS PASS` and a `gtin ok` line.
5. One build rc 0. The build log has no `gen_gfx_tab failed` line. `memcheck ... margin` above 0 KB. Delete this page. Commit build.sh, gtin.sh and the page delete.
Fix rights: build.sh line 98 and gtin.sh only. A FAIL with a named cause: 2 tries, then the stop rule (CLAUDE.md 5) with the output.
Stop: committed. Hand-back build3ds/out/handback_GTIN.txt, first 8 lines: commit hash, the step 2 output first line, `gtin ok`, host result, build rc, memcheck margin.
