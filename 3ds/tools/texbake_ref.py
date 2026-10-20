"""Extract the real CPU decode path from 3ds/src/pm_tex.c into a host header (so the test follows the source).
Usage: python3 texbake_ref.py <pm_tex.c> <out.h>
Takes the functions pot_at_least_8 rd16 rd16s write5551 gpu_fmt tiled16 wrap_mod expand, and the tile copy block of load_inner
(from 'const void* upload = scratch;' up to 'C3D_TexLoadImage')."""
import re, sys
src = open(sys.argv[1]).read()
def func(name):
    m = re.search(r"^(?:static |static inline )[^\n;]*\b" + name + r"\([^;{]*\{", src, re.M)
    if not m: sys.exit("function not found: " + name)
    i, d = m.end(), 1
    while d:
        c = src[i]; d += (c == "{") - (c == "}"); i += 1
    return src[m.start():i]
out = ["/* generated from pm_tex.c by texbake_ref.py */"]
for n in ("pot_at_least_8", "rd16", "rd16s", "write5551", "gpu_fmt", "tiled16", "wrap_mod", "expand", "nib", "floor4", "expand_dual"): out.append(func(n))
a = src.index("const void* upload = scratch;"); b = src.index("C3D_TexLoadImage(&s->tex, upload")
blk = src[a:b]
blk = blk.replace("if (tb_use) {", "if (0) {")  # the baked pack branch and the check block are not part of the decode
blk = blk[:blk.index("    if (tr && !tb_use")] if "    if (tr && !tb_use" in blk else blk
out.append("static const void* ref_tile(unsigned bpp, int pw, int ph) {\n" + blk + "    return upload;\n}")
open(sys.argv[2], "w").write("\n".join(out) + "\n")
