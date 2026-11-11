# Row DEADCODE: remove dead and redundant code from the 3dsx (and the future cia)

> STALE (oracle 2026-11-11): 3ds/src/pm_gbi.c no longer exists (its code moved to nd_*.c and pm_gpu*.c in DIRECT and MOVE); names and line numbers from it are void. The oracle re-reads this row before launch.

## User facts
User (2026-11-09): "use this as a chance to just clean up the 3dsx file and future cia file. We can remove dead or redundant code."
Also queued earlier: stale references after DIRECT (3ds/ld/3dsx.ld.in:34 names deleted .text.nd_walk_run/.text.pm_gbi_run;
pm_port.h:102 comment names nd_walk_run).

## Start state
`sh 3ds/tools/wt_new.sh DEADCODE`, branch DEADCODE, AFTER HC1 and HC2 merge (they edit the same draw files and the linker script).
PM_ROW=DEADCODE, PM_LANE=DEADCODE. One Haiku agent.

## Steps
1. Measure: the build already uses -ffunction-sections; check that the link uses --gc-sections. Add -Wl,--print-gc-sections to a
   census build and list what the linker drops. Then, with HC0's call graph (scratch tools named in plan_hotcode.md) and nm, list the
   functions and data that stay linked but are never called or read from any root (main, threads, callbacks, tables, scripts).
2. Classify each item: (a) dead port code (old walker remnants, deleted paths' helpers, unused switches); (b) N64-only code compiled
   for the 3DS (src/os, nusys parts the port replaced); (c) game code reached only through tables (keep); (d) debug tools (keep, but
   move to cold/-Os sections if hot).
3. Delete (a) and (b) in pieces, one commit per group, each build-checked. Fix the stale references above. No behaviour change.
4. Report .text, .rodata, .data and .bss sizes before/after (memcheck line and the map).

## Gates (light)
- nd_host PASS. Byte-equal against stored base shots: spot_mac01_det, battle_start_f182_cold.
- memcheck OK with the static size drop reported. No item of class (c) removed: a boot to the title and warp=random runs once.
