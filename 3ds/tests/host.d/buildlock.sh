# SLOTFIX: the build slot test (3ds/tests/buildlock/run.sh: 3 slot overlap, dead holder, docker cases, 12 builders on 4 slots
# with a planting reaper, the late-reaper interleaving) inside the host image.
sh 3ds/tests/buildlock/run.sh > /tmp/buildlock_hostd.txt 2>&1 || { tail -n 5 /tmp/buildlock_hostd.txt; exit 1; }
tail -n 3 /tmp/buildlock_hostd.txt
