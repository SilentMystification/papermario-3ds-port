"""xfer4_check.py - MTX0: per `xfer map ... f<N>` line and its xfer4 line, the o+c+n+t step sum must be N.
Usage: python 3ds/tools/xfer4_check.py [--seed] [log file]   (stdin when no file). --seed adds 1 to the first N (must FAIL).
Prints `xfer4 check OK <transitions>` (exit 0) or `xfer4 check FAIL n<k> f<N> sum <S>` (exit 1)."""
import re
import sys

args = sys.argv[1:]
seed = "--seed" in args
args = [a for a in args if a != "--seed"]
text = open(args[0], encoding="utf-8", errors="replace").read() if args else sys.stdin.read()
pend, seeded, ok = None, False, 0
for line in text.splitlines():
    m = re.search(r"xfer map \S+ f(\d+) ", line)
    if m:
        pend = int(m.group(1))
        if seed and not seeded:
            pend += 1
            seeded = True
        continue
    m = re.search(r"xfer4 map n(\d+) \| o (\d+)/\S+/\d+ c (\d+)/\S+/\d+ n (\d+)/\S+/\d+ t (\d+)/\S+/\d+", line)
    if m and pend is not None:
        s = sum(int(m.group(i)) for i in (2, 3, 4, 5))
        if s != pend:
            print("xfer4 check FAIL n%s f%d sum %d" % (m.group(1), pend, s))
            sys.exit(1)
        ok += 1
        pend = None
if ok == 0 or pend is not None:
    print("xfer4 check FAIL n0 f0 sum 0 (%d transitions)" % ok)
    sys.exit(1)
print("xfer4 check OK %d" % ok)
