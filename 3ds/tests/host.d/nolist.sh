#!/bin/sh
# NOLIST: nd_nolist.h poisons the list macro names, Gfx and gMainGfxPos (G8). Prints one line.
F="-m32 -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
T=$(mktemp -d /tmp/nolist.XXXXXX) || exit 1

fail() {
  echo "nolist FAIL $1"
  cat "$T"/*.out 2>/dev/null | head -40
  rm -rf "$T"
  exit 1
}

# good cases: must compile
printf '#include "nd_nolist.h"\nint nl_f(void) { return 0; }\n' > "$T/good1.c"
gcc $F -fsyntax-only "$T/good1.c" > "$T/good1.out" 2>&1 || fail good1
printf '#include "common.h"\n#include "nd_nolist.h"\nint nl_f(void) { return 0; }\n' > "$T/good2.c"
gcc $F -fsyntax-only "$T/good2.c" > "$T/good2.out" 2>&1 || fail good2
ng=2

# bad cases: must fail, and the output must hold "poisoned"
nb=0
for n in Gfx gMainGfxPos gDPPipeSync gsSPEndDisplayList gSPDisplayList; do
  if [ "$n" = Gfx ]; then
    body='Gfx* nl_p;'
  else
    body="void nl_f(void) { (void)&$n; }"
  fi
  printf '#include "common.h"\n#include "nd_nolist.h"\n%s\n' "$body" > "$T/bad_$n.c"
  if gcc $F -fsyntax-only "$T/bad_$n.c" > "$T/bad_$n.out" 2>&1; then
    fail "bad $n"
  fi
  grep -q poisoned "$T/bad_$n.out" || fail "bad $n"
  nb=$((nb+1))
done

rm -rf "$T"
echo "nolist ok good $ng bad $nb"
exit 0
