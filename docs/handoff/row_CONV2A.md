# Row CONV2A: the native table form v2 (NdNOp, NdNTab) in nd_gtab.h, next to v1

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main ec2519608)
- 3ds/include/nd_gtab.h (71 lines) holds the S7 v1 form: NdGOp (:19), NdGDef (:36), NdGTab (:44), NdGIdx (:51), NDG_DECL / NDG
  (:61-62), nd_rec_dl and nd_gtab_run (:68-69), `#endif` at :71. v1 ops point into list units (`&list[k]`). v2 holds no
  pointer into a Gfx array and no typed unit (plan v3 section 2).
- NdPso (uint16_t) and NdBindDesc are in 3ds/include/nd_api.h (NPSO1 merged). The names NdNOp, NdNTab and NDN_ are free.
Start: `sh 3ds/tools/wt_new.sh CONV2A`. PM_ROW=CONV2A PM_LANE=CONV2A on every call. Blocker NPSO1: merged.
## Touch only (places)
3ds/include/nd_gtab.h: new lines between :69 (`void nd_gtab_run(const uint32_t* rec);`) and :71 (`#endif`), and one
`#include "nd_api.h"` after :9. New files 3ds/tools/nd_ntab_test.c, 3ds/tests/host.d/ntab.sh. Nothing else.
## Exact edits
1. nd_gtab.h after :9 add `#include "nd_api.h"`. Before `#endif` add:
```
/* CONV2A (plan v3 section 2): the native table of one (static list, entry pso) pair. No pointer into a Gfx array, no typed unit.
 * The converter (CONV2C) writes the tables. nd_ntab_run (CONV2D) runs them on core 0 into the native emit. */
enum {
    NDN_END = 0,  /* end of the op list */
    NDN_PSO,      /* b = NdPso id of the pipeline state from here on */
    NDN_TEX,      /* a = tile (0 or 1), p = const NdBindDesc* */
    NDN_XF,       /* a = gSPMatrix flags (0xFF = pop one modelview matrix), p = const Mtx* (NULL: v = segmented address, input of the caller) */
    NDN_COL,      /* a = 0 prim, 1 env, 2 fog, 3 blend; v = RGBA; b = minlod << 8 | lodfrac for prim */
    NDN_DRAW,     /* p = const Vtx* base (NULL: v = segmented address), b = index count, a = 0; the indices follow in the table's idx from idx_pos */
    NDN_CALL      /* p = const NdNTab* of another (list, entry) pair */
};
typedef struct NdNOp { uint8_t op, a; uint16_t b; uint32_t v; const void* p; uint32_t idx_pos; } NdNOp;
typedef struct NdNTab {
    const void* list;     /* the static list this table replaces (the key with entry) */
    NdPso entry;          /* the pipeline state the caller states at nd_draw_list */
    uint16_t nops;
    const NdNOp* ops;     /* nops entries, the last one NDN_END */
    const uint16_t* idx;  /* the index buffer of every NDN_DRAW of the table (absolute vertex indices, CONV2B resolver) */
} NdNTab;
```
2. nd_ntab_test.c: `#include "nd_gtab.h"`, `#include <stdio.h>`, `#include <stddef.h>`. Static asserts (-m32): sizeof(NdNOp) == 16,
   offsetof(NdNOp, v) == 4, offsetof(NdNOp, p) == 8, sizeof(NdNTab) == 16. A fixture: one table with ops {NDN_PSO b 3},
   {NDN_COL a 0 v 0xFF8000FF}, {NDN_DRAW b 3 idx_pos 0}, {NDN_END} and idx {0, 1, 2}. main walks the ops to NDN_END and prints
   `ntab ok ops 4 draws 1 idx 3`.
3. host.d/ntab.sh (LF): `gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ntab 3ds/tools/nd_ntab_test.c && /tmp/ntab`.
4. Commit: `CONV2A: native table form v2 (NdNOp, NdNTab) in nd_gtab.h, host test`.
## Gate (expected values)
1. Seeded fault (a value change): in the test write `sizeof(NdNOp) == 12`. `python 3ds/tools/bench.py run nd_host --row CONV2A --why
   "CONV2A seeded fault"`: expected a compile error at that assert, nd_host FAIL. Write 16 again.
2. Same command, --why "CONV2A gate": `ntab ok ops 4 draws 1 idx 3`, `ND HOST TESTS PASS`.
3. `PM_ROW=CONV2A PM_LANE=CONV2A sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK (every file that includes
   nd_gtab.h compiles with the new include). No Azahar. Counter: none (design form of plan section 2).
## Stop and end
A failed gate (an include cycle of nd_api.h and nd_gtab.h included: name it), a line that does not match the text above, or a
tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
