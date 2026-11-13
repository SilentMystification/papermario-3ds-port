# Row FIXGOOD: good fixture, no fault

(Model: Haiku. Fixture for rowlint.py. One fault kind only.)
## Facts
- none.
## Touch only
Files: 3ds/tools/nd_gfx_conv.c. Symbols: v2_units_top in 3ds/tools/nd_gfx_conv.c.
New 3ds/tools/rowlint.py (test row). New names: none. Host link: none.
## Exact edits
1. 3ds/tools/nd_gfx_conv.c:13 `#include "nd_gtab.h"` is the include of the file.
## Gate
- rowlint on this file: faults 0, exit 0.
