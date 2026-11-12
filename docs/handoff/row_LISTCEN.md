# Row LISTCEN: census of all display-list code left (builders, data, readers) as a host bench

(Model: Haiku. Tool row: one new script and one bench entry. Every count rule is below. Why not smaller: none.)

## Why
The user's definition of done (2026-11-12) is no list builder, no list data, no list reader and no walker state model. This bench
counts each class per file, so every NATIVEAPI row reports what it removed and S9GATE checks for 0 (row_ORACLE_NEXT.md, chain
section).

## Start
`sh 3ds/tools/wt_new.sh LISTCEN`; PM_ROW=LISTCEN. No running row edits these files.

## Touch only
3ds/tools/listcen.py (new); 3ds/tools/bench_defs.json: one entry `listcen` inserted right after the `hotset_size` entry.

## Exact edits
1. listcen.py (python 3, standard library only, run from the repo root). Strip C comments (`/* */` and `//`) before matching.
   Classes and their regexes, counted per file:
   - `build`: in src/**/*.c and include/**/*.h, the matches of `\bg[DS]P[A-Za-z0-9_]*\s*\(`.
   - `gfxpos`: in src/**/*.c, the matches of `\bgMainGfxPos\b`.
   - `data`: in src/**/*.c, include/**/*.h and assets/**/*.c, the matches of `\bgs[DS]P[A-Za-z0-9_]*\s*\(`, and separately the
     arrays `^\s*(static\s+)?Gfx\s+\w+\s*\[` (count `data_arrays`).
   - `gfxtype`: in src, include and 3ds/src, the matches of `\bGfx\b`.
   - `reader`: in 3ds/src/*.c and 3ds/include/*.h, the matches of
     `\bNDO_[A-Z0-9_]+|\bG_(DL|VTX|TRI1|TRI2|TEXRECT|SETCOMBINE|SETOTHERMODE_[HL]|MTX|POPMTX)\b|\bnd_ops_run\b|\bnd_walk_state\b|\bnd_rec_state\b|\bNDC_STATE\b|\bENT_GBI\b`.
   - `model`: the function names of the walker state model, counted as defined functions (a `^\w[^;]*\b(name)\s*\(` line
     followed by `{`), from this list: sync_combine, nd_comb_decide, decide, decide_miss, sync_render, set_tilesize, tex_win0,
     tex_win1, ensure_tile, ensure_tile_slow, nd_texrect, nd_fillrect, gv_tri, op_setcombine, load_mtx.
2. Output: per class, the 15 largest files as `listcen file <class> <path> <count>`, then the totals, one line each:
   `listcen total build <n>`, `listcen total gfxpos <n>`, `listcen total data <n>`, `listcen total data_arrays <n>`,
   `listcen total gfxtype <n>`, `listcen total reader <n>`, `listcen total model <n>`. Exit 0.
3. bench_defs.json entry `listcen`: kind host_test, speed normal, default false, command `py -3 3ds/tools/listcen.py`, cap 60, tags
   tool and nativeapi. The purpose line: "NATIVEAPI end-state census: list builders, list data, list readers and walker model
   functions left (row_ORACLE_NEXT.md chain section); every class 0 is the S9GATE pass." log_metrics: one per total line, e.g.
   name `build`, regex `listcen total build (\\d+)`, pick last (7 metrics). gates: none.

## Gate
- `python 3ds/tools/bench.py rebuild` and `python 3ds/tools/bench.py run listcen --row LISTCEN --why "end-state census baseline"`:
  the run records 7 metrics.
- Expected order of size (oracle grep on main 529924217, without comment stripping): build about 4014 in src/*.c plus include,
  gfxpos about 4030, data about 8053 in assets plus src, data_arrays in about 953 files. If a total is 0 or more than 2 times
  these numbers, the regex is wrong: stop and hand back the totals.
- No Azahar start.

## Stop
A failed gate: stop, commit, hand back. Delete this row file in the completing commit. Hand back the 7 totals.
