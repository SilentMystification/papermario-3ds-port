#!/bin/sh
# GTIN: build.sh GT_IN covers every .c source of 3ds/tools/gconv_src.d (an edit to one rebuilds nd_gfx_tab.c). Seeded fault: the first GT_IN line alone must miss a source.
srcs=$(sed 's/#.*//' 3ds/tools/gconv_src.d/*.txt | tr -s ' \t\r' '\n' | grep '\.c$' | sort -u)
cover() { for p in $srcs; do ok=0; for w in $1; do case "$p" in "$w"|"$w"/*) ok=1; break;; esac; done; [ $ok = 1 ] || echo "$p"; done; }
GT_IN=; eval "$(grep '^ *GT_IN=' 3ds/build.sh)"
miss=$(cover "$GT_IN"); [ -z "$miss" ] || { echo "gtin FAIL: not in GT_IN: $miss"; exit 1; }
GT_IN=; eval "$(grep -m1 '^ *GT_IN=' 3ds/build.sh)"
[ -n "$(cover "$GT_IN")" ] || { echo "gtin seeded fault not seen"; exit 1; }
echo "gtin ok"
