# ws_LKEY_code: exact text for ws_LKEY.md (copy each block as it is)
## A: 3ds/include/nd_lkey.h (new file)
```c
/* LKEY: the typed units a list reader walks. NDN(x) (nd_gtab.h) and the v1 table of a moved list (NDG_F_MOVED: list = &ndd_<name>) point to an
 * NdNDev, not to list units: a reader that walks that pointer reads the struct as words (the VCOUNT crash: mdl_get_vertex_count). e is
 * nd_ntab_find(list) (nd_tab_init sorts the index before the logic thread starts: any thread may call it). */
#ifndef ND_LKEY_H
#define ND_LKEY_H
#include "nd_op.h"
#include "nd_gtab.h"
/* A list with a device entry e: e->u and at most e->nu units (no end unit). Any other list: the list itself and max (the reader stops at NDO_END). */
static inline const uint32_t* nd_lkey_units(const void* list, const NdNDev* e, unsigned max, unsigned* n) {
    if (e) {
        *n = e->nu < max ? (unsigned)e->nu : max;
        return e->u;
    }
    *n = max;
    return (const uint32_t*)list;
}
/* A v1 table (NdGTab): the units of its list, as nd_lkey_units. */
static inline const uint32_t* nd_lkey_tab(const NdGTab* t, unsigned max, unsigned* n) {
    return nd_lkey_units(t->list, (t->flags & NDG_F_MOVED) ? (const NdNDev*)(const void*)t->list : 0, max, n);
}
/* For a reader that needs the end unit (nd_dlc_tex_decode, nd_ent_segments): a list with a device entry is copied into buf (at most max - 1
 * units) and an NDO_END unit follows; buf has 2 * max words. Any other list: the list itself. */
static inline const uint32_t* nd_lkey_end(const void* list, const NdNDev* e, uint32_t* buf, unsigned max) {
    unsigned n, k;
    if (!e) return (const uint32_t*)list;
    const uint32_t* u = nd_lkey_units(list, e, max - 1u, &n);
    for (k = 0; k < n; k++) { buf[2u * k] = u[2u * k]; buf[2u * k + 1u] = u[2u * k + 1u]; }
    buf[2u * k] = NDO_W0(NDO_END, 0);
    buf[2u * k + 1u] = 0;
    return buf;
}
#endif
```
## B: 3ds/tools/nd_lkey_test.c (new file)
```c
/* LKEY host test: nd_lkey_units, nd_lkey_tab, nd_lkey_end (nd_lkey.h). A moved list key gives its device units (nu, no end unit); a plain list gives itself. */
#include <stdio.h>
#include <stdint.h>
#include "nd_lkey.h"

#define A 0x00100000u
/* the device units of a moved list: VTX, TRI2, NOP (nu 3). The 4th unit stands for the next array in rodata (a far VTX): never read. */
static const uint32_t mu[] = { ND_OP_VTX_W0(4, 0), A, NDO_W0(NDO_TRI2, 0), 0, ND_NOP_A, 0, ND_OP_VTX_W0(8, 0), 0x00be0000u };
static const NdNDev md = { &md, 0u, mu, 3u, 2u };
/* a plain list: VTX, END */
static const uint32_t pl[] = { ND_OP_VTX_W0(3, 0), A, NDO_W0(NDO_END, 0), 0 };

static unsigned vtx_count(const uint32_t* u, unsigned n) { /* the reader form of ent_vtx_range: stop at n or at NDO_END */
    unsigned v = 0;
    for (unsigned k = 0; k < n; k++) {
        if ((u[2u * k] >> 24) == NDO_END) break;
        if ((u[2u * k] >> 24) == NDO_VTX) v++;
    }
    return v;
}

int main(void) {
    unsigned n = 0;
    uint32_t buf[2u * 8u];
    const uint32_t* u = nd_lkey_units(&md, &md, 2048u, &n);
    if (u != mu || n != 3u || vtx_count(u, n) != 1u) { printf("lkey FAIL case 0\n"); return 1; }
    u = nd_lkey_units(pl, 0, 2048u, &n);
    if (u != pl || n != 2048u || vtx_count(u, n) != 1u) { printf("lkey FAIL case 1\n"); return 1; }
    u = nd_lkey_units(&md, &md, 2u, &n);
    if (u != mu || n != 2u) { printf("lkey FAIL case 2\n"); return 1; }
    const NdGTab tm = { (const uint32_t*)(const void*)&md, 0, 0, NDG_F_PARTS | NDG_F_MOVED, 0, 0 };
    const NdGTab tp = { pl, 0, 0, 0, 0, 0 };
    u = nd_lkey_tab(&tm, 4096u, &n);
    if (u != mu || n != 3u) { printf("lkey FAIL case 3\n"); return 1; }
    u = nd_lkey_tab(&tp, 4096u, &n);
    if (u != pl || n != 4096u) { printf("lkey FAIL case 4\n"); return 1; }
    u = nd_lkey_end(&md, &md, buf, 8u);
    if (u != buf || buf[0] != mu[0] || buf[4] != ND_NOP_A || (buf[6] >> 24) != NDO_END || vtx_count(u, 8u) != 1u) { printf("lkey FAIL case 5\n"); return 1; }
    u = nd_lkey_end(pl, 0, buf, 8u);
    if (u != pl) { printf("lkey FAIL case 6\n"); return 1; }
    printf("lkey ok\n");
    return 0;
}
```
## C: 3ds/tests/host.d/lkey.sh (new file)
```sh
#!/bin/sh
# LKEY: nd_lkey.h gives the device units of a moved list key (nd_lkey_test.c). The seed (no entry test) must print lkey FAIL case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/lkey_seed
sed 's/if (e) {/if (0) {/' 3ds/include/nd_lkey.h > /tmp/lkey_seed/nd_lkey.h
gcc -m32 -w -I /tmp/lkey_seed $F -o /tmp/lkey_seed/t 3ds/tools/nd_lkey_test.c || exit 1
got=$(/tmp/lkey_seed/t); if [ "$got" != "lkey FAIL case 0" ]; then echo "lkey seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/lkey_t 3ds/tools/nd_lkey_test.c && /tmp/lkey_t || exit 1
```
## D: src/entity_model.c (3 edits)
D1. Before (line 6): `#include "nd_geo.h"`. After: that line, then `#include "nd_lkey.h" /* LKEY: the units of a moved list key */`.
D2. Before (lines 281-290, inside ent_vtx_range):
```c
    for (int k = 0; k < 2048; k++) {
        uint32_t t[2];
        t[0] = dl[k].words.w0; t[1] = dl[k].words.w1;
        const u32 op = t[0] >> 24;
        if (op == (u32)NDO_END) break;
        if (op == (u32)NDO_VTX && (dl[k].words.w1 >> 24) == ((u32)seg & 0xffu)) {
            const u32 end = (dl[k].words.w1 & 0xffffffu) + (((dl[k].words.w0 >> 12) & 0xffu) * 16u);
            if (end > range) range = end;
        }
    }
```
After:
```c
    unsigned n;
    const uint32_t* u = nd_lkey_units(dl, nd_ntab_find(dl), 2048u, &n); /* LKEY: a moved list key (NDN) reads its device units */
    for (unsigned k = 0; k < n; k++) {
        const u32 w0 = u[2u * k], w1 = u[2u * k + 1u];
        const u32 op = w0 >> 24;
        if (op == (u32)NDO_END) break;
        if (op == (u32)NDO_VTX && (w1 >> 24) == ((u32)seg & 0xffu)) {
            const u32 end = (w1 & 0xffffffu) + (((w0 >> 12) & 0xffu) * 16u);
            if (end > range) range = end;
        }
    }
```
D3. Before (lines 321-324, inside entcheck_draw):
```c
        NdTexDec tdec;
        for (int k = 0; q != nullptr && k < 2048; k++) {
            uint32_t t[2];
            t[0] = q[k].words.w0; t[1] = q[k].words.w1;
```
After:
```c
        NdTexDec tdec;
        static uint32_t qb[2u * 256u]; /* LKEY: a moved list key (NDN) gives its device units and an end unit */
        const uint32_t* qu = q != nullptr ? nd_lkey_end(q, nd_ntab_find(q), qb, 256u) : nullptr;
        for (int k = 0; qu != nullptr && k < 2048; k++) {
            uint32_t t[2];
            t[0] = qu[2 * k]; t[1] = qu[2 * k + 1];
```
Before (line 337): `nd_dlc_tex_decode((const uint32_t*)(const void*)q, &tdec)`. After: `nd_dlc_tex_decode(qu, &tdec)` (the rest of the line stays).
## E: src/effects/energy_orb_wave.c (2 edits)
E1. Before (line 7): `#include "nd_op.h" /* HC4e2b: the typed unit of a list word */`. After: that line, then `#include "nd_lkey.h" /* LKEY */`.
E2. Before (line 258): `            const Gfx* g = NDN(D_09001000_3A5320);`. After (2 lines):
```c
            unsigned gn;
            const uint32_t* gu = nd_lkey_units(NDN(D_09001000_3A5320), nd_ntab_find(NDN(D_09001000_3A5320)), 64u, &gn); /* LKEY: the device units of the moved list */
```
Before (lines 261-267, from `            for (; n < 560; g++) {` to its closing `            }`). After:
```c
            for (unsigned k = 0; k < gn && n < 560; k++) {
                if ((gu[2u * k] >> 24) == NDO_END) break;
                n += snprintf(orbBuf + n, sizeof orbBuf - n, " %02x:%06x:%08x", (unsigned)(gu[2u * k] >> 24),
                              (unsigned)(gu[2u * k] & 0xFFFFFF), (unsigned)gu[2u * k + 1u]);
            }
```
## F: 3ds/src/nd_ent.c (6 edits)
F1. Before (line 16): `#include "nd_gtab.h"`. After: that line, then `#include "nd_lkey.h"`.
F2. Before (lines 113-115, ent_hash_walk):
```c
    for (unsigned i = 0; i < ND_DLC_MAXOPS; i++) {
        u32 w0 = l[i].words.w0, w1 = l[i].words.w1;
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
```
After:
```c
    unsigned nl;
    const uint32_t* lu = nd_lkey_units(l, nd_ntab_find(l), ND_DLC_MAXOPS, &nl); /* LKEY: a moved list key hashes its device units */
    for (unsigned i = 0; i < nl; i++) {
        u32 w0 = lu[2u * i], w1 = lu[2u * i + 1u];
        h = (h ^ w0) * 16777619u; h = (h ^ w1) * 16777619u;
```
F3. Before (line 133): `int ent_flat(const Gfx* l, unsigned* n, int depth) {`. After: `static int ent_flat_call(uint32_t w1, unsigned* n, int depth); /* LKEY: defined below */`, then the same line.
F4. Before (lines 140-141):
```c
            const Gfx* t = (const Gfx*)ptr_of(w1);
            if (!t || !is_host((u32)t) || depth >= 5 || !ent_flat(t, n, depth + 1)) return 0;
```
After: `            if (!ent_flat_call(w1, n, depth)) return 0; /* LKEY: a callee with a device entry (a moved list key) flattens from its units */`
F5. Before (lines 233-235, ent_tdwalk):
```c
    const uint32_t* L = t->list;
    const NdGOp* o = t->op;
    for (unsigned k = 0; k < ND_DLC_MAXOPS; ) {
```
After:
```c
    unsigned nL, k = 0;
    const uint32_t* L = nd_lkey_tab(t, ND_DLC_MAXOPS, &nL); /* LKEY: a moved list's table (NDG_F_MOVED) walks its device units (no end unit) */
    const NdGOp* o = t->op;
    for (; k < nL; ) {
```
Before (lines 258-260): `        (*flat)++; k++;` / `    }` / `    return -1;`. After: the first 2 lines, then
`    return (k == nL && nL < ND_DLC_MAXOPS) ? 0 : -1; /* LKEY: the end of the device units is the end unit */`
F6. Before (line 526):
`    if (w0 & G_PM_ENT_END) { pm_gbi_ent_texchk((const uint32_t*)(const void*)(uintptr_t)w1); pm_gbi_ent_segchk((const uint32_t*)(const void*)(uintptr_t)w1); return 0; }`
After:
```c
    if (w0 & G_PM_ENT_END) { /* LKEY: a moved list key (NDN) gives its device units and an end unit */
        static uint32_t eb[2u * 256u];
        const void* el = (const void*)(uintptr_t)w1;
        const uint32_t* eu = nd_lkey_end(el, nd_ntab_find(el), eb, 256u);
        pm_gbi_ent_texchk(eu); pm_gbi_ent_segchk(eu); return 0;
    }
```
## G: 3ds/src/nd_walk.c (2 edits)
G1. Before (line 20): `#include "nd_gtab.h"`. After: that line, then `#include "nd_lkey.h"`.
G2. Before (line 544): `    else nd_walk_state((const uint32_t*)c->list, ND_LIST_MAX); /* at the limit: the walker drops a deeper G_DL; its own check decides */`
After: `    else { unsigned nc; const uint32_t* cu = nd_lkey_tab(c, ND_LIST_MAX, &nc); nd_walk_state(cu, nc); } /* at the limit: the walker drops a deeper G_DL; its own check decides; LKEY: a moved list's units */`
## H: 3ds/src/nd_rec.c (2 edits; ModelRenderModes holds keys when the Gfx_RM lists move)
H1. Before (line 17): `#include "nd_gtab.h"`. After: that line, then `#include "nd_lkey.h"`.
H2. Before (lines 147-150, nd_rm_build):
```c
    unsigned n = 0;
    const Gfx* l = i < ND_RM_LISTS ? ModelRenderModes[i] : NULL;
    for (unsigned k = 0; l && n + 1u < ND_RM_OPS; k++) {
        const uint32_t w0 = l[k].words.w0, w1 = l[k].words.w1;
```
After:
```c
    unsigned n = 0, nl = 0;
    const Gfx* l = i < ND_RM_LISTS ? ModelRenderModes[i] : NULL;
    const uint32_t* lu = l ? nd_lkey_units(l, nd_ntab_find(l), 4096u, &nl) : NULL; /* LKEY: a moved render mode list (NDN) reads its device units */
    for (unsigned k = 0; k < nl && n + 1u < ND_RM_OPS; k++) {
        const uint32_t w0 = lu[2u * k], w1 = lu[2u * k + 1u];
```
## I: 3ds/tools/lkey_cen.py (new file)
```python
"""LKEY census: the list word readers. NDN(x) (nd_gtab.h) and a moved list's v1 table give an NdNDev, not list units: a reader that walks such a
pointer reads the struct as words (the VCOUNT crash). Every file with a reader line must be in the allow file with its count and a verdict.

Usage: lkey_cen.py <allow.txt> [root]
Reader line (comments stripped): src/**/*.c|h and include/**/*.h (not src/effects/gfx/): a read of `words.w0`/`words.w1` (not an assignment
target) or a cast `(const uint32_t*)`/`(uint32_t*)`/`(u32*)` of a name with Gfx, gfx, dl, list, g, l or q in it; 3ds/src/*.c: the `words.w0`/`w1`
read or a read of `<name>->list`. allow.txt: `<path> <count> <verdict> <reason>`; verdict KEY (the file calls nd_lkey_ or nd_ntab_find), MAP (map
shape lists only), LOCAL (its own stack, heap or indexed array), REC (record words), HOLD (a list gset.py keeps: SetCustomGfx, BranchList, X[]),
OTHER (not a Gfx list). Prints `lkey census ok files <n> lines <m>` (exit 0) or one `lkey census bad <path> ...` line per fault and `lkey census FAIL` (exit 1)."""
import pathlib
import re
import sys

VERDICTS = ("KEY", "MAP", "LOCAL", "REC", "HOLD", "OTHER")
R_WORD = re.compile(r"words\s*\.\s*w[01]\b(?!\s*=[^=])")
R_CAST = re.compile(r"\((?:const\s+)?(?:uint32_t|u32)\s*\*\)\s*(?:\(const\s+void\s*\*\)\s*)?\(?\s*(?:\w+(?:->|\.))*\w*(?:[Gg]fx|[Dd]l|[Ll]ist|\bg\b|\bl\b|\bq\b)")
R_LIST = re.compile(r"\w+->list\b(?!\s*=[^=])")
R_KEY = re.compile(r"\bnd_lkey_\w+\s*\(|\bnd_ntab_find\s*\(")
R_COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def strip(text):
    return R_COMMENT.sub(lambda m: "\n" * m.group(0).count("\n"), text)


def count(rel, text):
    lines = strip(text).split("\n")
    if rel.startswith("3ds/src/"):
        return sum(1 for ln in lines if R_WORD.search(ln) or R_LIST.search(ln))
    return sum(1 for ln in lines if R_WORD.search(ln) or R_CAST.search(ln))


def census(root):
    out = {}
    pats = ("src/**/*.c", "src/**/*.h", "include/**/*.h", "3ds/src/*.c")
    for pat in pats:
        for f in sorted(root.glob(pat)):
            rel = f.relative_to(root).as_posix()
            if rel.startswith("src/effects/gfx/") or not f.is_file():
                continue
            text = f.read_text(encoding="utf-8", errors="replace")
            n = count(rel, text)
            if n:
                out[rel] = (n, text)
    return out


def main(argv):
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    root = pathlib.Path(argv[2]) if len(argv) > 2 else pathlib.Path(".")
    allow = {}
    for ln in pathlib.Path(argv[1]).read_text(encoding="utf-8").replace("\r", "").split("\n"):
        if not ln.strip() or ln.lstrip().startswith("#"):
            continue
        p = ln.split(None, 3)
        allow[p[0]] = (int(p[1]), p[2])
    got = census(root)
    bad = 0
    for rel, (n, text) in sorted(got.items()):
        if rel not in allow:
            print(f"lkey census bad {rel} {n} not listed"); bad += 1; continue
        an, v = allow[rel]
        if v not in VERDICTS:
            print(f"lkey census bad {rel} verdict {v}"); bad += 1
        if n != an:
            print(f"lkey census bad {rel} {n} listed {an}"); bad += 1
        if v == "KEY" and not R_KEY.search(strip(text)):
            print(f"lkey census bad {rel} KEY without nd_lkey_ or nd_ntab_find"); bad += 1
    for rel in sorted(set(allow) - set(got)):
        print(f"lkey census bad {rel} listed, no reader line"); bad += 1
    if bad:
        print("lkey census FAIL")
        return 1
    print(f"lkey census ok files {len(got)} lines {sum(n for n, _ in got.values())}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
```
## J: census files (group 2)
J1. 3ds/tests/lkey/allow.txt (new file). `<n>` = the count that the census prints for that path (`lkey census bad <path> <count> listed 0`): write 0 first, run, copy the count.
```text
# LKEY census (3ds/tools/lkey_cen.py): <path> <count> <verdict> <reason>. A new reader line fails host.d/lcen.sh: the oracle names its verdict first.
3ds/src/nd_direct.c 3 LOCAL kFillKind combine words (nd_direct.c:262,279-280)
3ds/src/nd_ent.c <n> KEY ent_hash_walk, ent_flat_call, ent_tdwalk, nd_ent_case through nd_lkey_ (LKEY); ent_draw and ent_refcheck read records and the flat copy
3ds/src/nd_gtab.c 1 KEY nd_rec_dl: nd_ntab_find before nd_gt_state_only (nd_gtab.c:119-121)
3ds/src/nd_rec.c <n> KEY nd_rm_build through nd_lkey_units (LKEY); nd_rec_list reads SetCustomGfx lists (HOLD, gset.py:158); stray_word reads nd_rec_top
3ds/src/nd_recs.c 61 REC model record words (model.c appendGfx_model)
3ds/src/nd_seam.c 9 OTHER NdSeamList tables, not a Gfx list
3ds/src/nd_shape.c 8 MAP map shape lists (dlc_try takes only gMapShapeData, nd_shape.c:878) and seam tables
3ds/src/nd_walk.c <n> KEY walk_call nd_ntab_find (nd_walk.c:230), gt_call nd_lkey_tab (LKEY), NDG_F_MOVED (nd_walk.c:441,489), record words
src/animator.c 1 KEY MESH1 nd_ntab_find (animator.c:879)
src/draw_box.c 1 HOLD gBoxCombineModes[] is indexed (gset.py:158 keeps X[)
src/effects/effect_3D.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/embers.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/pink_sparkles.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/shimmer_burst.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/snaking_static.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/snowfall.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/effects/water_block.c 1 LOCAL Gfx g[] on the stack for nd_parts_add
src/entity_model.c <n> KEY ent_vtx_range, entcheck_draw through nd_lkey_ (LKEY); entRec record words
src/imgfx.c 1 LOCAL state->gfxBufs: heap copies of ROM data (imgfx.c:1260)
src/model.c 10 KEY mdl_get_vertex_count nd_ntab_find (VCOUNT); MAP mdl_local_gfx_update_vtx_pointers and mdl_project_tex_coords (displayData->displayList); LOCAL record words
src/world/area_omo/omo_02/barricade.c 1 MAP displayData->displayList (barricade.c:114)
src/world/common/util/GetFirstTriangleNormal.inc.c 1 MAP displayData->displayList (mim_01, mim_03, mim_06 hint.c:13)
```
If the census prints a path that is not in J1 (or energy_orb_wave.c): stop and send the output to the oracle. Do not add a line yourself.
J2. 3ds/tests/lkey/case_new/allow.txt (new file): one line `# LKEY census seed: no file is listed`.
J3. 3ds/tests/lkey/case_new/src/x.c (new file): 2 lines `/* LKEY census seed: one list word reader that allow.txt does not list. */` and
`unsigned x_read(const Gfx* g) { return g[0].words.w0; }`
J4. 3ds/tests/host.d/lcen.sh (new file):
```sh
#!/bin/sh
# LKEY: the list reader census (lkey_cen.py). The tree must match 3ds/tests/lkey/allow.txt. The seed (case_new: one reader line, not listed) must fail.
PY=${PY:-python3}
$PY 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt . || exit 1
got=$($PY 3ds/tools/lkey_cen.py 3ds/tests/lkey/case_new/allow.txt 3ds/tests/lkey/case_new | tail -1)
if [ "$got" != "lkey census FAIL" ]; then echo "lkey census seed not seen: [$got]"; exit 1; fi
echo "lkey census test ok"
```
