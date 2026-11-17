#!/bin/sh
# ENTSRC: gen_ntab_entries.py reads an extra file with a namespace (N(x) -> <NS>_x) and resolves ENTITY_ADDR(e, Gfx*, x); entapply.py
# takes a site in any src/ directory. Seeded faults: ENTITY_ADDR renamed in a copy of gen_ntab_entries.py, src/effects only in a copy
# of entapply.py. Each must change the output. Prints one line; exit 1 on a fail.
R=$(pwd)
T=${TMPDIR:-/tmp}/entsrc.$$
rm -rf "$T"; mkdir -p "$T/src/entity"
python3 -I 3ds/tools/gen_ntab_entries.py 3ds/tests/entsrc 3ds/tests/entsrc/x/w.c=dro_02 > "$T/gne.txt" || { echo "entsrc FAIL gne exit"; exit 1; }
if ! tr -d '\r' < 3ds/tests/entsrc/a.txt | cmp -s "$T/gne.txt" -; then echo "entsrc FAIL gne output differs from a.txt"; diff "$T/gne.txt" 3ds/tests/entsrc/a.txt | head; exit 1; fi
sed 's/ENTITY_ADDR/ENTITY_ADDX/' 3ds/tools/gen_ntab_entries.py > "$T/gne_fault.py"
python3 -I "$T/gne_fault.py" 3ds/tests/entsrc 3ds/tests/entsrc/x/w.c=dro_02 > "$T/gne_fault.txt" || true
if tr -d '\r' < 3ds/tests/entsrc/a.txt | cmp -s "$T/gne_fault.txt" -; then echo "entsrc FAIL gne seeded fault not seen"; exit 1; fi
printf '#include "nd_gtab.h"\nvoid f(void) {\n    nd_rec_dl(NDN(Ent_A));\n}\n' > "$T/src/entity/E.c"
cp "$T/src/entity/E.c" "$T/E.orig"
printf 'ndd_Ent_A f /w/src/entity/E.c:3 00000001 00000002 00000003 00000004 00000000 00000000 00000000 00000000 00000001 00000000 00000000\n' > "$T/log.txt"
ea=$(cd "$T" && python3 -I "$R/3ds/tools/entapply.py" log.txt | tr -d '\r' | tr '\n' '|')
l3=$(sed -n 4p "$T/src/entity/E.c" | tr -d '\r'); l2=$(sed -n 2p "$T/src/entity/E.c" | cut -c1-31)
cp "$T/E.orig" "$T/src/entity/E.c"
sed 's|(src/(?:|(src/effects/(?:|' 3ds/tools/entapply.py > "$T/ea_fault.py"
ef=$(cd "$T" && python3 -I "$T/ea_fault.py" log.txt | tr -d '\r' | tr '\n' '|')
rm -rf "$T"
case $ea in *"entapply site src/entity/E.c:3 pe_E_3|"*"entapply sites 1 "*) ;; *) echo "entsrc FAIL entapply: [$ea]"; exit 1 ;; esac
if [ "$l3" != "    nd_draw_list(NDN(Ent_A), pe_E_3);" ] || [ "$l2" != "ND_PSO_DESC(pe_E_3, 0x00000001," ]; then echo "entsrc FAIL entapply edit: [$l2] [$l3]"; exit 1; fi
case $ef in *"entapply sites 0 "*) ;; *) echo "entsrc FAIL entapply seeded fault not seen: [$ef]"; exit 1 ;; esac
echo "entsrc ok"
