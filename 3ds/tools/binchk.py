#!/usr/bin/env python3
"""binchk.py <elf> <string> [<string> ...] | <elf> --file <list>: count each string in the built binary.
Exit 0 when every count is above 0, exit 1 when one is 0. Use before a console push: the binary must hold the merged code
(build 5877 lacked STARTLR2 and MOON7, user fact 2026-11-16). Pick a string the row adds (a log text, a menu label)."""
import sys

def main(argv):
    if len(argv) < 3:
        print("usage: binchk.py <elf> <string> [<string> ...]", file=sys.stderr)
        return 2
    data = open(argv[1], "rb").read()
    strs = list(argv[2:])
    if len(strs) == 2 and strs[0] == "--file":  # one string per line, lines starting with # are comments
        strs = [l.rstrip("\r\n") for l in open(strs[1], encoding="utf-8") if l.strip() and not l.startswith("#")]
    bad = 0
    for s in strs:
        n = data.count(s.encode("utf-8"))
        print(f"binchk {n:5d}  {s}")
        if n == 0:
            bad += 1
    print("binchk FAIL: %d string(s) missing" % bad if bad else "binchk OK")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
