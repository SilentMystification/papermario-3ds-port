# ws_FXVROM_code: exact text for ws_FXVROM.md (copy each block as it is)
## A: build3ds/out/fxv_edit.py (save it, run `python build3ds/out/fxv_edit.py .` once; it asserts each old text once and prints `fxv edit ok`)
```python
import pathlib, sys
R = pathlib.Path(sys.argv[1])
def rep(rel, old, new):
    p = R / rel
    s = p.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in s else "\n"
    o, n = old.replace("\n", nl), new.replace("\n", nl)
    assert s.count(o) == 1, (rel, s.count(o), old[:60])
    p.write_bytes(s.replace(o, n).encode("utf-8"))

# A: nd_gconv.h helpers
rep("3ds/tools/nd_gconv.h", "#define gconv_nsym ((unsigned)(__stop_gconv_sym - __start_gconv_sym))\n",
"""#define gconv_nsym ((unsigned)(__stop_gconv_sym - __start_gconv_sym))
/* FXVROM: the device keeps the Vtx arrays of a source in ROM order. A name D_<seg>_<ROM hex> gives the ROM offset of the array; the next
 * array in ROM order starts at that offset + 16 * length. No libc: the gconv_tu files include this header after the game headers. */
static inline unsigned gconv_vtx_len(const GSym* s) { return (unsigned)(((const char*)s->end - (const char*)s->addr) / 16); }
static inline int gconv_rom_of(const char* nm, unsigned long* r) {
    const char* u = 0;
    if (nm[0] != 'D' || nm[1] != '_') return 0;
    for (const char* p = nm; *p; p++) if (*p == '_') u = p;
    if (!u[1]) return 0;
    unsigned long v = 0;
    for (const char* p = u + 1; *p; p++) {
        const int c = *p;
        const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (d < 0) return 0;
        v = v * 16ul + (unsigned long)d;
    }
    *r = v;
    return 1;
}
static inline const GSym* gconv_vtx_rom_next(const GSym* s) {
    unsigned long r, q;
    if (!gconv_rom_of(s->name, &r)) return 0;
    r += 16ul * gconv_vtx_len(s);
    for (unsigned i = 0; i < gconv_nsym; i++) if (gconv_sym[i].kind == GS_VTX && gconv_rom_of(gconv_sym[i].name, &q) && q == r) return &gconv_sym[i];
    return 0;
}
""")
# B: converter macro
rep("3ds/tools/nd_gfx_conv.c", "static int is_draw(unsigned id) {",
"""#ifndef ROM_SPLIT
#define ROM_SPLIT 1 /* FXVROM: vrom.sh builds a copy with -DROM_SPLIT=0 and expects the unsup line back */
#endif
static int is_draw(unsigned id) {""")
# C: converter split
rep("3ds/tools/nd_gfx_conv.c", """            o.kind = ND_VS_VTX; o.n = (uint8_t)n; o.v0 = (uint8_t)(e - n); o.arr = (uint32_t)(vs - gconv_sym); o.first = off / 16u;
            v2_seg_add(&o);""",
"""            o.kind = ND_VS_VTX; o.n = (uint8_t)n; o.v0 = (uint8_t)(e - n); o.arr = (uint32_t)(vs - gconv_sym); o.first = off / 16u;
            while (ROM_SPLIT && o.first + o.n > gconv_vtx_len(vs)) { /* FXVROM: a load past its array end reads the next array in ROM order */
                const GSym* nx = gconv_vtx_rom_next(vs);
                if (!nx) break; /* no ROM name or no next array: the load stays whole (v2_flush checks its refs) */
                const unsigned c = gconv_vtx_len(vs) - o.first;
                if (c) { NdVsOp p = o; p.n = (uint8_t)c; v2_seg_add(&p); }
                o.n = (uint8_t)(o.n - c); o.v0 = (uint8_t)(o.v0 + c); o.arr = (uint32_t)(nx - gconv_sym); o.first = 0; vs = nx;
            }
            v2_seg_add(&o);""")
# D: host compare normalisation
rep("3ds/tools/nd_gtab_test.c", """        e->base[j] = nl ? mslot[s[j]].base : NULL;
        e->i[j] = nl ? mslot[s[j]].idx : s[j];
""", """        e->base[j] = nl ? mslot[s[j]].base : NULL;
        e->i[j] = nl ? mslot[s[j]].idx : s[j];
        const GSym* vs = nl ? m_vsym((uint32_t)(uintptr_t)e->base[j]) : NULL; /* FXVROM: an index past the array end is in the next array in ROM order */
        while (vs && e->i[j] >= gconv_vtx_len(vs)) {
            const GSym* nx = gconv_vtx_rom_next(vs);
            if (!nx) break;
            e->i[j] -= gconv_vtx_len(vs); vs = nx; e->base[j] = nx->addr;
        }
""")
print("fxv edit ok")
```
## B: 3ds/tests/host.d/vrom.sh (new)
```sh
#!/bin/sh
# FXVROM: a vertex load past its array end splits in ROM order (nd_gfx_conv.c ROM_SPLIT). D_09002CE8_3C0688 (snowman_doll.c: a 30-vertex
# load on the 9-vertex D_090015E0_3BEF80) has a v2 table. Seeded fault: the converter built with -DROM_SPLIT=0 on the same objects
# (/tmp/gconv of nd_host_test.sh) reports the list unsup again. Prints `vrom ok` or fails.
T=/tmp/gconv; V=/tmp/vrom; rm -rf "$V"; mkdir -p "$V"
grep -qx D_09002CE8_3C0688 "$T/ntab_tables.txt" || { echo "vrom FAIL: D_09002CE8_3C0688 has no v2 table"; exit 1; }
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
gcc -m32 -Wall -Wextra -Werror -DROM_SPLIT=0 -DND_SCOMP_HOST $DEF -I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include \
    -o "$V/conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c 3ds/tools/nd_vslot.c "$T/gconv_in.o" -Wl,--gc-sections
"$V/conv" "$V/d.c" "$V/i.txt" "$V/u.txt" "$V/x.c" "$V/t.txt" "$T/ntab_entries.txt" "$V/p.txt" "$V/g.txt" > "$V/tab.c" 2> "$V/err.txt" || true
grep -q "^D_09002CE8_3C0688: vertex ref outside the set" "$V/u.txt" || { echo "vrom seeded fault not seen"; exit 1; }
echo "vrom ok"
```
