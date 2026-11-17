# HOSTSEL: seeded faults of the per-unit result cache (3ds/tools/hostsel.py selftest): a read file edit misses, an unread file
# edit hits, a changed stored byte or read list misses (the entry goes), a trace without its own script is not stored.
python3 3ds/tools/hostsel.py selftest 2>/dev/null || { echo "hostsel FAIL"; exit 1; }
