# Row LOADMTX: load_mtx skips a matrix source outside host memory and warns once (user decision 2026-11-11)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts
- ESCAPE3 run 249036 (attract demo after a title return): 96 UnmappedAccess Read32 @ 0xF51F30DE..0xF51F311A at PC 0x00100630 =
  load_mtx (nd_mtx_set.c:104-111: nd_mtx_unpack of the Mtx at ptr_of(w1)). HOTORD base run 245748 (cold boot): Read32 @ 0x01..0x2D
  at the same PC (src = 1). On the console an unmapped read is a data abort.
- No "gbi: G_MTX ... outside host memory" line (nd_walk.c:36-39, gbi_cold_bad_mtx) in run 249036: the walker arm (nd_walk.c:285-288)
  did not see it, so the callers were the record paths rec_matrix / rec_apply (nd_recs.c:67, :69, :96, :101: R2 w1 = model finalMtx,
  R8 w1 = the billboard matrix).
## Cause: not proven (note)
- The bad reads, the HC4e4 run 238358 crash and the battle_start_f182_cold base fault ("two runs differ", base --fill at b8bc06613,
  both runs -NoWarm cold) all show on cold boots. A warm restart reboots the emulated system (fresh, zeroed process memory), so no
  .bss or heap content survives it: uninitialized memory does not explain "cold only". What differs on a cold boot is host timing
  (shader cache build), so the thread interleaving differs: the ROM stream (pm_romload.c rs_loader, demand reads pm_rs_need,
  pm_os.c:427-460), the core 1 asset prefetch (pm_map.c:177-181 pm_prefetch_take, MAPX-4) and the logic/walker queue. A race there can
  leave a record or model word stale. This is the next oracle question; row LINRACE (linear allocator race from the prefetch thread)
  is the nearest open row.
- battle_start_f182_cold base fault: a game (determinism) fault, not a tool fault: one slot, one build, both cold, both log_pass,
  shots differ. No tool row. Do not use it as a byte gate until the race is found.
## Start
`sh 3ds/tools/wt_new.sh LOADMTX` (build3ds/wt/LOADMTX, branch LOADMTX from main). PM_ROW=LOADMTX PM_LANE=LOADMTX on every call.
Runs beside HC8 (pm_gpu.c, nd_model.c, nd_ent.c, nd_shape.c) and SYNC60 (pm_gpu.c pm_gpu_begin_wait): no shared file.
## Touch only
3ds/src/nd_mtx_set.c: load_mtx (:97-111) and one new cold function above it. No other function, no other file.
## Exact edits
1. Above `void load_mtx(u32 w0, u32 w1, u32 hdr) {` add:
```
/* LOADMTX: a matrix source outside host memory (a stale or wrong record word). The load is skipped: the old matrix stays. One warning per run,
 * as the walker's G_MTX check (nd_walk.c gbi_cold_bad_mtx). pm_log_evt passes the logging gate in every mode (console log, prof mode). */
static __attribute__((cold, noinline)) void mtx_bad(u32 w0, u32 w1, u32 hdr, const void* src, const void* caller) {
    static int once;
    if (once) return;
    once = 1;
    pm_log_evt("load_mtx: source %08x outside host memory, w0 %08x w1 %08x hdr %08x, caller %p, frame %u, skipped",
               (unsigned)(uintptr_t)src, (unsigned)w0, (unsigned)w1, (unsigned)hdr, caller, pm_frame_count());
}
```
2. In load_mtx, after `if (!src) return;` add
   `if (__builtin_expect(!is_host((u32)(uintptr_t)src), 0)) { mtx_bad(w0, w1, hdr, src, __builtin_return_address(0)); return; } /* LOADMTX */`
   is_host (nd_walk.c:430) is declared in nd_int.h:66; pm_log_evt and pm_frame_count in pm_port.h (both included at :7-12 via nd.h and
   pm_port.h). If the build reports an undeclared name, add only that declaration at the top of the file.
## Gate (CLAUDE.md 2: nd_host and byte compare)
- `PM_ROW=LOADMTX PM_LANE=LOADMTX sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row LOADMTX --why "LOADMTX guard"`: PASS.
- For spot_mac01_det and mdl5_iwa10_det (bases stored at b8bc06613): `python 3ds/tools/bench.py base <bench> --row LOADMTX`,
  `python 3ds/tools/bench.py run <bench> --row LOADMTX --lane LOADMTX --why "LOADMTX byte gate"`, `python 3ds/tools/bench.py cmp <bench>
  --row LOADMTX`: 6/6 byte-equal each. One Azahar list, 2 starts. A "crash symbols" rc 2 with a load_mtx PC: hand back last_crash.txt.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
