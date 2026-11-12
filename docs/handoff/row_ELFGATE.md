# Row ELFGATE: the ELF half of the end proof (3ds/tools/elfgate.py; row_ENDGAME.md section 4)

(Model: Haiku. A standalone Python script with fixed patterns; host only. It is the check script now; the final gate run is the
last row of the plan.)

## Facts (ORACLE7, main 235bb8721)
- build3ds/pm_3ds.elf (the main build) and build3ds/lane-<row>/pm_3ds.elf hold a .symtab (the map shows .bss.D_800B32E0 etc.).
- The Gfx array names come from source: `Gfx NAME[` and `Gfx N(name)[` definitions in src/**/*.c, src/**/*.inc.c and the
  `#include "*.gfx.inc.c"` files under assets/us. N(name) expands per map namespace (each world map's `#define NAMESPACE <map>`
  in its header gives `<map>_name`).

## Start
`sh 3ds/tools/wt_new.sh ELFGATE`. PM_ROW=ELFGATE.

## Touch only
New 3ds/tools/elfgate.py, new 3ds/tools/elfgate_patterns.txt.

## Exact edits
1. elfgate_patterns.txt: one regex per line, `func ` or `data ` first, copied from row_ENDGAME.md section 4 (the function list
   and the data list). A name with a GCC clone suffix (`.constprop.N`, `.isra.N`, `.part.N`, `.cold`) is matched by its base name.
2. elfgate.py `<elf>`: parse the ELF32 little-endian header, the section headers, .symtab and .strtab with `struct` only (no
   pip package). For every symbol: name, value, size, type (FUNC or OBJECT), section name. Count `func` matches (type FUNC),
   `data` matches (type OBJECT), and `gfxnames`: OBJECT symbols whose name is in the Gfx name set of the source scan (the scan of
   the Facts; N(name) gets the namespace of the file's map header, else it is skipped and counted as `gfxscan_skip`). Print up
   to 20 matched names per class, then `elfgate func <n> data <m> gfxnames <k> gfxscan_skip <s>`. Exit 0 always.

## Gate (expected values)
- `python 3ds/tools/elfgate.py build3ds/pm_3ds.elf > build3ds/out/elfgate.txt`: func above 0 (nd_walk_state and nd_ops_run are
  linked today), data above 0 (the ndg_ tables), gfxnames above 1000 (most Gfx arrays are linked today). Hand back the line.
- No Azahar, no build. Budget 0.

## Stop
A parse error on the main ELF: stop, commit, hand back the traceback. Delete this row file in the completing commit.
