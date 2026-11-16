#!/usr/bin/env python3
"""EVAR: every list an EVAR site can draw (the value set of its variable) is a self-contained v2 table (a line of ntab_tables.txt).
Usage: evarset.py [--seed] <ntab_tables.txt>   (run from the repo root)
The value sets (oracle code read): the single lists below and the members of the fragment arrays in src/entity/model.
--seed adds Entity_Tweester_RenderInnerWhirl (an inheriting table, ntab_inherit.txt): the check must then fail.
Last line: `evarset lists <n> self <s> bad <b>`. Exit 1 when b > 0 or n is not 173."""
import re
import sys

SINGLE = ['Entity_Chest_RenderLid', 'Gfx_HiddenPanel_RenderHole', 'Entity_SaveBlock_RenderBlock', 'Entity_Padlock_RenderShackle',
          'Entity_PadlockRedFrame_RenderShackle', 'Entity_PadlockRedFace_RenderShackle', 'Entity_PadlockBlueFace_RenderShackle',
          'Entity_ArrowSign_RenderRotatedSign', 'D_0A000D18_E9D618', 'Entity_StarBoxLauncher_RenderBottom']
ARRAYS = [('src/entity/model/WoodenCrate.c', 'Entity_WoodenCrate_FragmentsRender'),
          ('src/entity/model/BoardedFloor.c', 'Entity_BoardedFloor_FragmentsRender'),
          ('src/entity/model/BombableRock.c', 'Entity_BombableRock_FragmentsRender'),
          ('src/entity/model/ShatteringHammer1Block.c', 'Entity_ShatteringHammer1Block_FragmentsRender'),
          ('src/entity/model/ShatteringHammer2Block.c', 'Entity_ShatteringHammer2Block_FragmentsRender'),
          ('src/entity/model/ShatteringHammer3Block.c', 'Entity_ShatteringHammer3Block_FragmentsRender'),
          ('src/entity/model/ShatteringBrickBlock.c', 'Entity_ShatteringBrickBlock_FragmentsRender')]


def main(argv):
    seed = '--seed' in argv
    args = [a for a in argv if a != '--seed']
    tabs = set(open(args[0], encoding='utf-8').read().split())
    names = list(SINGLE)
    for path, arr in ARRAYS:
        s = open(path, encoding='utf-8').read()
        m = re.search(r'Gfx\*\s+' + arr + r'\[\]\s*=\s*\{(.*?)\};', s, re.S)
        if not m:
            print('evarset no array %s %s' % (path, arr))
            return 1
        for x in m.group(1).split(','):
            x = x.strip()
            if not x:
                continue
            n = re.fullmatch(r'NDN\((\w+)\)', x)
            names.append(n.group(1) if n else x)
    if seed:
        names[0] = 'Entity_Tweester_RenderInnerWhirl'
    bad = [x for x in names if x not in tabs]
    for x in bad:
        print('evarset not self %s' % x)
    print('evarset lists %d self %d bad %d' % (len(names), len(names) - len(bad), len(bad)))
    return 1 if bad or len(names) != 173 else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
