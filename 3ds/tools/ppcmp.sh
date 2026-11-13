#!/bin/sh
# ppcmp.sh <outdir> <file>...: preprocessed view of each file (row N64DEL1). gcc -E -P with the device defines (DEF) and
# include paths (INC) of gen_gfx_tab.sh, in one docker run of pm3ds-host:1 with the nd_host_test.sh mounts. Writes
# <outdir>/<file with / replaced by _>.i. Exit 1 when a gcc call fails.
cd "$(dirname "$0")/../.." || exit 2
ROOT=$(pwd -W 2>/dev/null || pwd)
# The main worktree holds assets/us and ver/us/build (git-ignored, read only, mounted at /ar).
AR=$(cd "$(git rev-parse --path-format=absolute --git-common-dir)/.." && pwd -W 2>/dev/null || pwd)
OUT=$1
shift
if [ -z "$OUT" ] || [ $# -eq 0 ]; then echo "usage: ppcmp.sh <outdir> <file>..." >&2; exit 2; fi
mkdir -p "$ROOT/$OUT" || exit 2
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
INC="-I 3ds/tools/gconv_inc -I 3ds/tools -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -I ."
MSYS_NO_PATHCONV=1 docker run --rm -v "$ROOT:/w" -v "$AR/assets:/ar/assets:ro" -v "$AR/ver/us/build:/ar/ver/us/build:ro" -w /work pm3ds-host:1 sh -c '
  out=$1; shift
  mkdir -p "/w/$out" || exit 2
  mkdir -p /work/ver/us
  for d in 3ds include src; do ln -s "/w/$d" "/work/$d"; done
  ln -s /ar/assets /work/assets
  ln -s /ar/ver/us/build /work/ver/us/build
  fail=0
  for f in "$@"; do
    o="/w/$out/$(echo "$f" | sed "s#/#_#g").i"
    if gcc -E -P -dD -w -Wno-builtin-macro-redefined -D__LINE__=0 '"$DEF"' '"$INC"' "$f" > /tmp/pp.raw; then
      grep -v "^[[:space:]]*$" /tmp/pp.raw > "$o"
    else
      echo "FAIL gcc -E $f"; fail=1
    fi
  done
  exit $fail
' sh "$OUT" "$@"
