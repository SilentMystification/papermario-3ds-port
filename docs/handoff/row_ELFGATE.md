# Row ELFGATE: the ELF half of the end proof (3ds/tools/elfgate.py; row_ENDGAME.md section 4)

(Model: Haiku. A standalone Python script with fixed patterns; host only. It is the check script now; the final gate run is the
last row of the plan.)

## Facts (ORACLE7, main 235bb8721. Correction 1: ORACLE8, main 5a53dd0a2, after the hand-back of 579b0500f)
- build3ds/pm_3ds.elf (the main build) and build3ds/lane-<row>/pm_3ds.elf hold a .symtab (the map shows .bss.D_800B32E0 etc.).
- Row fault (correction 1): assets/us is git-ignored, so a worktree has none. 3ds/build.sh:15-17 reads it from the main folder:
  `MAIN_ROOT=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)`, ASSET_BASE = "." when ./assets/us exists, else MAIN_ROOT.
  The scan of 579b0500f found 965 names because the worktree has no assets/us, and its include following missed files.
- Gfx definitions (ORACLE8 scan of main): every *.c file (also *.inc.c) under src and assets/us, each file read on its own, no
  include following. Plain `Gfx NAME[`: 1877 names. `Gfx N(name)[`: 204 definitions in 29 files. N(sym) is
  NS(NAMESPACE, NAME_PREFIX, sym, NAME_SUFFIX) (include/macros.h:45) = NAMESPACE_ + NAME_PREFIX + sym + NAME_SUFFIX.
  NAMESPACE resolves for 200 definitions, 4 stay unresolved (shared .inc.c files: ToadHouseBlanketAnim.inc.c, two vine.gfx.inc.c,
  blanket.gfx.inc.c). Name set 2076. On build3ds/pm_3ds.elf of the main folder (sha1 5bbaca326c95..., 2026-11-12 17:18:58),
  1806 of them are OBJECT symbols.

## Start
`sh 3ds/tools/wt_new.sh ELFGATE`. PM_ROW=ELFGATE.

## Touch only
New 3ds/tools/elfgate.py, new 3ds/tools/elfgate_patterns.txt.

## Exact edits
1. elfgate_patterns.txt: one regex per line, `func ` or `data ` first, copied from row_ENDGAME.md section 4 (the function list
   and the data list). A name with a GCC clone suffix (`.constprop.N`, `.isra.N`, `.part.N`, `.cold`) is matched by its base name.
2. elfgate.py `<elf>`: parse the ELF32 little-endian header, the section headers, .symtab and .strtab with `struct` only (no
   pip package). For every symbol: name, value, size, type (FUNC or OBJECT), section name. Count `func` matches (type FUNC),
   `data` matches (type OBJECT), and `gfxnames`: OBJECT symbols whose name is in the Gfx name set. Print up to 20 matched names
   per class, then `elfgate func <n> data <m> gfxnames <k> gfxscan_skip <s>`. Exit 0 always.
3. The Gfx name set (correction 1, replaces the include-following scan): src = the repo of the tool (two levels up). Asset root =
   that repo when it has assets/us, else the main folder `git rev-parse --git-common-dir` + "/.." (as 3ds/build.sh:15-17).
   Scan every file whose name ends in `.c` under <repo>/src and <asset root>/assets/us, each file on its own (no include
   following). Regex on the file text: `\bGfx\s+(?:N\((\w+)\)|(\w+))\s*\[`. Group 2: a plain name, add it. Group 1: N(name).
   NAMESPACE = the first `#\s*define\s+NAMESPACE\s+(\w+)` of the file, else of the first file named by an `#include "..."` of
   the file (path relative to the file's folder, in include order, files that exist) that has one. No NAMESPACE: count 1 in
   gfxscan_skip. NAME_PREFIX and NAME_SUFFIX: the value of the last `^[ \t]*#[ \t]*define[ \t]+NAME_(PREFIX|SUFFIX)[ \t]*(\w*)` line
   above the definition in the same file (empty when none). Name = NAMESPACE + "_" + prefix + name + suffix.
   Print `elfgate gfxset <n> gfxscan_files <f>` before the result line.

## Gate (expected values, correction 1)
- `python 3ds/tools/elfgate.py C:/Users/Andrew/Documents/GitHub/papermario-3ds-port/build3ds/pm_3ds.elf > build3ds/out/elfgate.txt`
  (the main folder ELF) and `sha1sum` of that ELF. Expected: gfxset 2076, gfxscan_skip 4, func above 0, data above 0, and
  gfxnames 1806 when the sha1 starts 5bbaca326c95. A newer main ELF (other sha1): gfxset and gfxscan_skip as above, hand back
  gfxnames with the sha1 (not a failure).
- No Azahar, no build. Budget 0.

## Stop
A parse error on the main ELF: stop, commit, hand back the traceback. Delete this row file in the completing commit.
