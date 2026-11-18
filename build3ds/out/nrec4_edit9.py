import re
p = "3ds/src/nd_snap.c"
s = open(p, "rb").read().decode("utf-8")
def rep(s, old, new):
    old = old.replace("\n", "\r\n"); new = new.replace("\n", "\r\n")
    assert s.count(old) == 1, (s.count(old), old[:60])
    return s.replace(old, new)
rect = "void nd_snapchk_rect(void) { nd_snap_arm = 0; if (PMDL(\"snapchk\")) sc_compare(); }   /* NDR0E2: a texture rectangle: compare only */\n"
s = rep(s, rect, rect +
 "static unsigned pr_cmp, pr_bad; /* NREC4: the particle compares (no combine apply) */\n"
 "void nd_snapchk_part(void) { nd_snap_arm = 0; if (!PMDL(\"snapchk\")) return; const unsigned b0 = sc_bad; sc_compare(); pr_cmp++; pr_bad += sc_bad != b0; } /* NREC4 */\n")
tsn = "        pm_log_evt(\"tsnap f%u cmp %u wtss %u tris %u vtss %u\", pm_frame_n, ts_cmp, ts_bad, ts_vn, ts_vb); /* NCONV2A */\n"
s = rep(s, tsn, tsn + "        pm_log_evt(\"psnap f%u cmp %u bad %u\", pm_frame_n, pr_cmp, pr_bad); /* NREC4 */\n")
open(p, "wb").write(s.encode("utf-8"))
p = "3ds/include/nd_snap.h"
s = open(p, "rb").read().decode("utf-8")
s = rep(s, "void nd_snapchk_rect(void);\n", "void nd_snapchk_rect(void);\n void nd_snapchk_part(void); /* NREC4 */\n")
open(p, "wb").write(s.encode("utf-8"))
p = "3ds/src/nd_geom.c"
s = open(p, "rb").read().decode("utf-8")
s = rep(s, "if (sn) { nd_snapchk_tri(); if (ep_epoch", "if (sn) { nd_snapchk_part(); if (ep_epoch")
open(p, "wb").write(s.encode("utf-8"))
print("ok")
