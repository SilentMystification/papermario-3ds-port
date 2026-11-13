# Row FIXNEEDS: needs fixture

(Model: Haiku. Fixture for rowlint.py. One fault kind only.)
## Facts
- none.
## Touch only
Files: 3ds/tools/nd_gfx_conv.c. New names: none. Host link: none.
## Exact edits
1. Edit 3ds/tools/nd_gfx_conv.c.
## Gate
Needs: docs/handoff/row_SEEDS.md has | live
- rowlint on this file: faults 1 (needs), exit 1.
