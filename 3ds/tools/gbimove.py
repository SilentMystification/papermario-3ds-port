"""gbimove.py <out> <guard> <src>... (DEL4D1): move each #define block of each <src> whose name starts with gDP, gSP, gsDP, gsSP (or _gDP...)
or whose text holds a gDP/gSP/gsDP/gsSP call or the word Gfx (KEEP excepted) to <out>, with the conditional lines around it (the include
guard of a <src>: its first and last conditional line, stays out). <out> starts with #include_next <PR/gbi.h>. Line endings are kept."""
import re, sys, pathlib
NAME, DFN = re.compile(r'[ \t]*#[ \t]*define[ \t]+_?gs?[DS]P'), re.compile(r'[ \t]*#[ \t]*define[ \t]+(\w+)')
USE, CND = re.compile(r'\bgs?[DS]P\w*\s*\(|\bGfx\b'), re.compile(r'[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b')
KEEP = {'gPmEntDrawRecord'}  # a device user: nd_rec.c nd_rec_ent_draw
out, guard, move = pathlib.Path(sys.argv[1]), sys.argv[2], []
for s in map(pathlib.Path, sys.argv[3:]):
    L = s.read_bytes().decode('latin-1').splitlines(keepends=True)
    cl = [i for i, l in enumerate(L) if CND.match(l)]
    keep, i, n, m0 = [], 0, 0, len(move)
    while i < len(L):
        j, d = i, DFN.match(L[i])
        if d:
            while L[j].rstrip().endswith(chr(92)) and j + 1 < len(L): j += 1
        blk = L[i:j + 1]
        if d and d.group(1) not in KEEP and (NAME.match(L[i]) or USE.search(''.join(blk))): move += blk; n += 1
        else:
            keep += blk
            if CND.match(L[i]) and i not in (cl[0], cl[-1]): move.append(L[i])
        i = j + 1
    s.write_bytes(''.join(keep).encode('latin-1'))
    print('gbimove %s blocks %d moved %d kept %d' % (s.as_posix(), n, len(move) - m0, len(keep)))
e = '\r\n'
head = ['/* gbi_inc/PR/gbi.h (DEL4D1, written by 3ds/tools/gbimove.py): the gbi macros for the host tools only (gconv, host tests). */' + e,
        '#ifndef ' + guard + e, '#define ' + guard + e, '#include_next <PR/gbi.h>' + e]
out.parent.mkdir(parents=True, exist_ok=True)
out.write_bytes(''.join(head + move + ['#endif' + e]).encode('latin-1'))
