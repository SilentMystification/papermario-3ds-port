## STATE
- Branch nds1. BENCH7 done (commits fcb8c0b0e build-at console, d61425cb7 push reconnect and attach). BENCH6 done before.
- build-at <commit> <console bench> --dry-run prints the 3 commands. Real run: lane at_<short> build, PM_SRC3DSX push to 192.168.1.170, ingest dirty false. The user runs the first real push.
- push_3ds.sh --attach --ip <ip> -t <s>: read a running game. Early end (exit 3 of livelog.py) reconnects with --append, 3 times.
- Open: build.sh caps lane volume sets (PM_LANE_KEEP 5); a new lane can evict another lane's volumes. Check before a build-at run.
- Next command: python 3ds/tools/bench.py list
