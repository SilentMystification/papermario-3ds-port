"""test_capcmp.py - host test of capcmp.compare on embedded log strings (no Azahar). Prints a line, exit 1 on fail."""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capcmp as cp  # noqa: E402


def block(F, ft="42.18", trect="33.0", kb="106", cls_ent="1.72", chg="0", ndc=True):
    """One prof window of frame F (the lines of the real logs, the tags capcmp reads)."""
    L = ["#1 prof f%d w300 fps 46.6 | ft %s max 82.3 o34 280 wk 34.01" % (F, ft),
         "#2 prof f%d w300 cls rec 1.06 ent %s spr 0.51" % (F, cls_ent)]
    if ndc:
        L.append("#3 prof f%d w300 ndc model 95.0 spr 6.0 ent 11.0 call 0.0 trect %s st 910.0 sw 670.0 kb %s" % (F, trect, kb))
    L += ["#4 prof f%d w300 nd 23.24 lg 23.60 dr 43 tc 0 tw 0 tri 180 leg 0 gb 20.57" % F,
          "#5 prof f%d w300 at mac_01 1:2 m4 sp-128 b0 e0 chg%s pos0 0 0 200 pos 0 0 200 disp w1 s0 f60" % (F, chg)]
    return "\n".join(L) + "\n"


def run(a_text, b_text):
    d = tempfile.mkdtemp()
    pa, pb_ = os.path.join(d, "a.txt"), os.path.join(d, "b.txt")
    with open(pa, "w") as fh:
        fh.write(a_text)
    with open(pb_, "w") as fh:
        fh.write(b_text)
    return cp.compare(pa, pb_)


def need(lines, *subs):
    for s in subs:
        assert any(s in ln for ln in lines), "missing %r in %r" % (s, lines)


def main():
    # T1 equal signature, ft 42.18 and 38.98
    r = run(block(689, ft="42.18"), block(689, ft="38.98"))
    need(r, "b f689 = a f689", "ft 42.18 38.98 -3.20")
    assert r[-1] == "capcmp matched 1/1 d ft mean -3.20", r
    # T2 seeded fault: trect 19.3 in b
    r = run(block(689), block(689, trect="19.3"))
    need(r, "b f689 none", "trect 33.0/19.3")
    assert r[-1] == "capcmp matched 0/1 d ft mean -", r
    # T3 only kb differs: match
    r = run(block(689), block(689, kb="99"))
    need(r, "b f689 = a f689")
    assert r[-1] == "capcmp matched 1/1 d ft mean +0.00", r
    # T4 an a window with chg1: no match
    r = run(block(689, chg="1"), block(689))
    assert not any("= a f" in ln for ln in r), r
    assert r[-1] == "capcmp matched 0/1 d ft mean -", r
    # T5 b without ndc: no ndc, not counted
    r = run(block(689), block(689, ndc=False))
    need(r, "b f689 no ndc")
    assert r[-1] == "capcmp matched 0/0 d ft mean -", r
    # T6 cls ent differs in both logs (ndc ent equal): match
    r = run(block(689, cls_ent="1.72"), block(689, cls_ent="2.50"))
    need(r, "b f689 = a f689")
    assert r[-1] == "capcmp matched 1/1 d ft mean +0.00", r
    print("captool capcmp ok")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as e:
        print("FAIL: %s" % (e,))
        sys.exit(1)
