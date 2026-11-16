"""test_capture_check.py - host test of capture_check.words_from_dry and judge (no Azahar). Prints a line, exit 1 on fail."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capture_check as cc  # noqa: E402

DRY = ("dry run: nothing built, nothing sent\n3dslink argv: /opt/devkitpro/tools/bin/3dslink -a 192.168.1.131 "
       "build3ds/pm_3ds.3dsx -- --verbose livelog grablog fbbind log=on prof=off\n")


def main():
    w = cc.words_from_dry(DRY)
    assert w == "livelog grablog fbbind log=on prof=off", w
    assert cc.words_from_dry("no argv line\n") is None
    ok, d = cc.judge("#5 options: levels run log_on=1 live=3\n#9 grabw f1\n#10 fbbind n0\n#11 grabw f2\n", ["grabw", "fbbind"])
    assert ok and d == "log gate on, proof grabw 2 fbbind 1", d
    ok, d = cc.judge("#1 build 5824-x git 6ef6af53c7 loglevel 3\n#2 fxtest: done\n", ["grabw"])  # seed: log_on 0 drops the gate line
    assert not ok and d == 'log gate OFF (no "options: levels run log_on=1" line), missing proof: grabw', d
    print("captool capture_check ok")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as e:
        print("FAIL: %s" % (e,))
        sys.exit(1)
