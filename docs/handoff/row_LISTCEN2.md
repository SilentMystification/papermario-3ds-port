# Row LISTCEN2: census v2 (the counters C1-C10 of row_ENDGAME.md, active vs N64-only, per file group)

(Model: Haiku. One Python tool extended by a fixed algorithm; host only, no Azahar. Plan: row_ENDGAME.md section 3.)

## Code read (ORACLE7, main 235bb8721)
- 3ds/tools/listcen.py counts build, gfxpos, data, data_arrays, gfxtype, reader, model over all lines; it does not split code
  compiled for the 3DS from N64-only branches, prints only the 15 largest files per class, and has no group view.
- The oracle's reference counts with the algorithm below (main 235bb8721): build active 3820, build n64 648, gfxpos active 3254,
  gfxpos n64 715, data 22103, Gfx arrays 2107, rec_dl 270 (src only; include and assets as listed in row_ENDGAME.md section 1).

## Start
`sh 3ds/tools/wt_new.sh LISTCEN2`. PM_ROW=LISTCEN2.

## Touch only
3ds/tools/listcen.py, 3ds/tools/bench_defs.json (the `listcen` bench entry only, if its parser needs the new line names).

## Exact edits (listcen.py)
1. `active_lines(text)` on the comment-stripped text: a stack of [kind, taking]. `#ifdef TARGET_3DS` -> [T, True];
   `#ifndef TARGET_3DS` and `#if !defined(TARGET_3DS)` -> [T, False]; `#if defined(TARGET_3DS)` without `&&`/`||` -> [T, True];
   any other `#if/#ifdef/#ifndef` -> [U, True]; `#else` flips taking only for T; `#elif` makes the top [U, True]; `#endif` pops.
   A line is active when every taking flag is True. Directive lines count as neither.
2. Classes per file, each with an `_n64` twin for inactive lines: build `\bg[DS]P\w*\s*\(`, gfxpos `\bgMainGfxPos\b`, data
   `\bgs[DS]P\w*\s*\(`, arrays `^\s*(static\s+)?(const\s+)?Gfx\s+[\w()]+\s*\[`, gfxtype `\bGfx\b`, recdl `\bnd_rec_dl\s*\(|\bnd_scomp_rec\s*\(`.
   Files: src/**/*.c, src/**/*.h, include/**/*.h, assets/**/*.c (as today). reader and model stay as today but print every file.
3. Groups: src/effects, src/effects/gfx, src/world, src/entity, src/battle, src/pause, src/filemenu, include, assets/<dir>, and
   one group per root src file. Output lines `listcen group <group> <class> <n>` for every non-zero value, then the existing
   `listcen file` lines (keep TOP 15) and `listcen total <class> <n>` for every class including the `_n64` twins.
4. Option `--group <prefix>`: print `listcen file <class> <path> <n>` for every file under the prefix (no TOP limit).

## Gate (expected values)
- `python 3ds/tools/listcen.py > build3ds/out/listcen2.txt`: totals build 3820 and build_n64 648, gfxpos 3254 and gfxpos_n64 715,
  data 22103, arrays 2107 (each within 5 of these: hand back the exact values and say which differ).
- `python 3ds/tools/listcen.py --group src/entity`: Tweester.c build 42 first.
- `bench.py run listcen --row LISTCEN2 --why "census v2"`: passes (it ingests the totals). No Azahar start. Budget 0.

## Stop
A total off by more than 5 from the reference: stop, commit, hand back both values and three files of the difference. Delete
this row file in the completing commit.
