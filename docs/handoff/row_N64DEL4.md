# Row N64DEL4: remove by hand the TARGET_3DS directives that unifdef3ds.py skips (7 skip blocks, 2 #elif chains)

Blocked by N64DEL2 and N64DEL3 (same files; their edits move the line numbers). Find each block by its directive text below.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13, ORACLE2 checked the
lines at 5c30af943.)
## Facts (ORACLE8, main cff324713; line numbers are main's)
- The device build always defines TARGET_3DS. It never defines SHIFT, AVOID_UB, SPLAT, __CTX__, PERMUTER or ND_GBI_WORDS (no
  -D in 3ds/CMakeLists.txt, the *.cmake files or ppcmp.sh DEF :13, and no `#define ND_GBI_WORDS` in the tree). So each form
  below has one live branch under the device defines.
- After N64DEL1-3 these 9 are the only directive lines left that name TARGET_3DS in include/ and src/.
## Touch only (places)
include/PR/os_convert.h, include/common.h, include/common_structs.h, include/include_asm.h, include/macros.h, src/entity.c,
src/battle/area/kzn2/actor/lava_piranha.c, src/battle/common/stage/lib/WaterEffects.inc.c,
src/world/common/lava_piranha/part1.inc.c. Only the blocks below.
New names: none. Host link: none (no 3ds/src file).
Needs: 3ds/tools/ppcmp.sh has -D__LINE__=0
## Exact edits (keep the live lines byte for byte, delete the directive lines and the dead branches)
1. os_convert.h:74-84 `#if defined(TARGET_3DS)` / `#elif !defined(__MWERKS__)` / `#else` / `#endif`: keep :75-77 (the comment and
   the 2 OS_K?_TO_PHYSICAL defines of the 3DS branch).
2. common.h:20-24 `#if defined(TARGET_3DS) && !defined(ND_GBI_WORDS)`: keep :23 `#include "nd_gbi.h"` and one comment line
   above it: `/* DIRECT D5: the dynamic gbi macros record frame stream commands. */`.
3. common_structs.h:2258-2269 `#if defined(TARGET_3DS)` / `#elif VERSION_JP` / `#elif VERSION_IQUE || VERSION_PAL` / `#else` /
   `#endif`: keep :2259 `#define DISPLAYCONTEXT_MTX_COUNT 0x800`.
4. include_asm.h:7-27 `#if !defined(SPLAT) && !defined(__CTX__) && !defined(PERMUTER) && !defined(TARGET_3DS)`: keep :26
   `#define INCLUDE_ASM(TYPE, FOLDER, NAME, ARGS...)`.
5. macros.h:75-82 `#ifdef DEBUG` / `#elif defined(TARGET_3DS)` / `#else` / `#endif`: :77 becomes `#else`. Delete :80-81 (the N64
   `#else` and its define). DEBUG stays a switch.
6. entity.c:8-29 `#ifdef SHIFT` / `#elif defined(TARGET_3DS)` (:16) / `#else` / `#endif`: keep the 6 lines of the TARGET_3DS branch
   (:17-22, from `extern u8 pm_world_entity_heap[];` to `#define entity_default_VRAM`).
7. lava_piranha.c:292-306 `#if defined(SHIFT) || defined(TARGET_3DS) // ...`: keep :293-300.
8. WaterEffects.inc.c:12-20 `#if defined(AVOID_UB) || defined(TARGET_3DS)`: keep :13-15 (their indent stays).
9. part1.inc.c:9-23 `#if defined(SHIFT) || defined(TARGET_3DS) // ...`: keep :10-17.
10. Commit: `N64DEL4: the last TARGET_3DS directives of include and src by hand (skip forms of unifdef3ds)`.
## Gate (expected values; ORACLE2 gate rule: only the changed files and their includers, 1 build, 0 Azahar)
1. Before the edits: `sh 3ds/tools/ppcmp.sh build3ds/out/pp_before <the 7 files that are not .inc.c> src/main.c src/model.c
   src/hud_element.c src/battle/area/kzn2/area.c src/battle/area/kgr/stage/kgr_01.c src/world/area_kmr/kmr_22/npc.c` (a .inc.c
   goes through its includer, as in N64DEL2 correction 2). After: the same into build3ds/out/pp_after. `diff -r` of both prints nothing. Seeded fault first:
   append `int ppcmp_seed;` to one pp_after file. diff names it. Delete the line.
2. `git grep -nE '^\s*#\s*(if|ifdef|ifndef|elif).*TARGET_3DS' -- include src` prints nothing.
3. `python 3ds/tools/listcen.py`: the active totals are unchanged against the run before edit 1 (hand back both).
4. `PM_ROW=N64DEL4 PM_LANE=N64DEL4 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. nd_host PASS. Budget 0 Azahar.
   Counter: the TARGET_3DS directives of include/ and src/ go to 0 (phase H end condition).
## Stop and end
A ppcmp difference, a directive left by gate 2, a build error, or a tool fault: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
