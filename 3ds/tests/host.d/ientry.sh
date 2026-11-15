L=Entity_Signpost_RenderSign
chk() { grep -q "^$L: itab (" "$1" && grep -qx "$L" /tmp/gconv/gset_ok.txt && ! grep -qx "$L" /tmp/gconv/ntab_tables.txt; }
chk /tmp/gconv/ntab_inherit.txt || { echo "ientry FAIL: $L has no itab table, or it is in ntab_tables"; exit 1; }
grep -v "^$L: itab" /tmp/gconv/ntab_inherit.txt > /tmp/ientry_fault.txt; chk /tmp/ientry_fault.txt && { echo "ientry FAIL: seeded fault not seen"; exit 1; }
echo "ientry ok"
