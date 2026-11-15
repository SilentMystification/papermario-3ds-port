grep -q "^D_09000F88_327398 D_09000F88_327398_e" /tmp/gconv/ntab_pairs.txt || { echo "iseed FAIL: D_09000F88_327398 has no call seed"; exit 1; }
echo "iseed ok"
