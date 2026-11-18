# ws_DEL1A (Haiku): the runners outside the walker name no walker op and no nd_ops_run (stage 1A: nd_rec.c, nd_recs.c, nd_shape.c, pm_nusys.c). nd_ops_run is nd_nr_list. Counters: C8 reader 341 -> 321, C7 gfxtype 91 -> 87 (FACT: listcen rules per line, main f2ab50d38d), C11 elf func -2 (nd_ops_run, gbi_task_w; FACT elfgate basebin 309a79ac9ab7e378). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. WAIT: branch from main after the DEL3A merge (DEL3A writes nd_walk.c :758, next to :757 here). Runs at the same time as DEL5B2: DEL5B2 writes nd_rec.c :365-394 and nd_walk.c :420-660; this row writes nd_rec.c :529-640 and nd_walk.c :169, :186, :225, :373-374, :757. Worktree build3ds/wt/DEL1A, branch DEL1A. PM_ROW=DEL1A PM_LANE=DEL1A.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_rec.c, nd_recs.c, nd_shape.c, nd_walk.c, pm_nusys.c; 3ds/include/nd.h, nd_nr.h; 3ds/ld/hot_order.ld; 3ds/tests/lkey/allow.txt (count only). Not here: nd_st.c :59 and the other stage 3 files (DEL3C).
## Facts (code read, main f2ab50d38d; find the quoted text, line numbers move)
- FACT nd_texrect reads only the low 24 bits of the rectangle w0 (nd_rect.c:226-227). NDC_TEXRECT p[2] has no other reader (nd_tex_set.c:231-233). So the G_TEXRECT byte of nd_rec.c:542, :621 and nd_nr.h:33 is never read.
- FACT nd_tx_test compares the macro form (nd_gx_w) with the setter form (ND_NR_TR_W0) (nd_tx_test.c:1-2): edits 1 and 4 change both the same way. ND_X0 gives the rectangle unit `NDO_W0(NDO_RECT, w0)` (nd_op.h:109).
- FACT every unit of a static list is typed (nd_rec.c:618): the G_ cases of nd_rec_list (:619-627) are a translation only; G_RDP*SYNC never reaches the switch.
- FACT gbi_walk and gbi_task_w are static in pm_nusys.c (:89, :100; calls :105, :130, :133, :200).
## Edits
1. nd_rec.c nd_gx_w: `tr_w[0] = ((uint32_t)G_TEXRECT << 24) | (w0 & 0xffffffu); tr_w[1] = ...` -> `tr_w[0] = w0; tr_w[1] = ...` (the comment: `the rectangle unit w0 and w1 without the flag (DEL1A)`).
2. nd_rec.c: list_bad takes `const uint32_t* g` and logs `(unsigned)g[0]`. nd_rec_list (:610-639) becomes:
```c
void nd_rec_list(const void* l) { /* DEL1A: the typed units of the list, no G_ translation */
    uint32_t kb[2u * 64u];
    const NdNDev* ke = nd_ntab_find(l);
    const uint32_t* g = (const uint32_t*)(ke == (const NdNDev*)l ? (const void*)nd_lkey_end(l, ke, kb, 64u) : l); /* CGFX: a moved key (NDN) */
    for (unsigned n = 0; n < 1024u; n++, g += 2) {
        const uint32_t w0 = g[0], w1 = g[1];
        const unsigned op = w0 >> 24;
        if (!nd_op_typed(op)) { list_bad(g, n); return; } /* HC4e4: a static list is typed at compile time */
        if (op == ND_FS_END) return;
        if (IS_NOP(w0)) continue; /* a sync or a no-op */
        if (op == ND_FS_RECT && (w1 & ND_FS_RECT_TEX)) { nd_rec_texrect(w0, w1 & ~ND_FS_RECT_TEX, g[3], g[5]); g += 4; n += 2; continue; }
        if (op == ND_FS_CALL && (w0 & 1u)) { /* a branch: host addresses only (a segment base is walker state) */
            if (!is_host(w1)) { list_bad(g, n); return; }
            g = (const uint32_t*)(uintptr_t)w1 - 2;
            continue;
        }
        if (op == ND_FS_CALL && rec_inline(w1)) continue;
        nd_rec_nr(w0, w1);
    }
    list_bad(g, 1024u);
}
```
   Keep the comment block above it (:603-605).
3. Rename the definition `void nd_ops_run(const void* l, unsigned n, int keep)` (nd_walk.c:374) to `void nd_nr_list(const void* l, unsigned n, int keep)` with a comment `/* DEL1A: was nd_ops_run */`. Every call `nd_ops_run(` -> `nd_nr_list(`: nd_walk.c :169, :186, :225 (2), :373, :757; nd_recs.c :45, :59; nd_shape.c :871. nd.h:555 the prototype. hot_order.ld:163 `*(.text.nd_ops_run)` -> `*(.text.nd_nr_list)`. Comments that name nd_ops_run may stay.
4. nd_recs.c rec_dl: `(u32)G_DL << 24` -> `ND_FS_W0(ND_FS_CALL, 0u)` (the w0 of the log only). nd_nr.h:33 -> `#define ND_NR_TR_W0(xh, yh) ND_FS_W0(ND_FS_RECT, ND_NR_XY12(xh, yh)) /* DEL1A: the rectangle unit form (nd_texrect reads the low 24 bits) */`.
5. pm_nusys.c: rename `gbi_walk` to `stream_walk` (:89, :105) and `gbi_task_w` to `stream_task` (:100, :130, :133, :200). Comments may stay.
6. `git grep -nwE "nd_ops_run|gbi_task_w|gbi_walk" -- 3ds/src 3ds/include 3ds/ld > build3ds/out/del1a_grep.txt`: only comment lines. `python 3ds/tools/listcen.py > build3ds/out/del1a_listcen.txt`: reader file lines nd_rec.c, nd_recs.c, nd_shape.c, nd.h, nd_nr.h gone; nd_walk.c 60.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-6. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del1a_host.txt 2>&1`: PASS. Commit `DEL1A 1: runners name no walker op; nd_ops_run is nd_nr_list`.
2. `python 3ds/tools/bench.py base kkj14_cgfx_drec --commit $(git merge-base HEAD main) > build3ds/out/del1a_base1.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row DEL1A --bench kkj14_cgfx_drec --no-host --why "DEL1A static list record" > build3ds/out/del1a_loop1.txt 2>&1`.
3. The same 2 commands for spot_mac01_drec (del1a_base2.txt, del1a_loop2.txt). `python 3ds/tools/elfgate.py build3ds/lane-DEL1A/pm_3ds.elf > build3ds/out/del1a_elf.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). del1a_grep.txt holds comments only. listcen reader 321 (the start commit value less 20), gfxtype 4 below the start commit. del1a_elf.txt names neither nd_ops_run nor gbi_task_w.
## Fix rights and stop
Fix rights: the listed files; compile errors; a host test whose expected texrect word holds the G_TEXRECT byte (set the byte to the ND_FS_RECT unit, name it in the commit). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call; `sed -b` only.
## Hand-back
Last commit `DEL1A 2: gate. kkj14_cgfx and spot_mac01 drec equal; reader -20` deletes this page. One SubagentHandback: commit hash, listcen totals, the elfgate line.
