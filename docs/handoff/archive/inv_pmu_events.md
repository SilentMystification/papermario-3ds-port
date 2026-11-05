# inv_pmu_events: meaning of the pmu and sp0c PMU fields

Scope: read only. No source, Azahar, or git changes.

## Where the events are set

- `3ds/src/pm_phase.c:240-250` `pmu_set_window()`: sets PMN0 and PMN1 with `PERFCOUNTEROP_SET_EVENT`, zeroes PMN0, PMN1 and the cycle counter.
- `3ds/src/pm_phase.c:242-243`: window A (`pmu_win` 0) uses ev0 = `PERFCOUNTEREVT_CORE_INST_CACHE_MISS`, ev1 = `PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS`. Window B (`pmu_win` 1) uses ev0 = `PERFCOUNTEREVT_CORE_MAIN_TLB_MISS`, ev1 = `PERFCOUNTEREVT_CORE_STALL_BY_LACK_OF_INST`.
- `3ds/src/pm_phase.c:265-269` `pmu_frame_end()`: prints `pmu f%u ev %u %u per frame ...` with the enum values, every 300 frames. Windows toggle at each print.
- `3ds/src/pm_phase.c:275-282` `pm_pmu_sp0()`: CAP2 read of the same counters, divided by `pmu_n`. Called from `3ds/src/pm_gbi.c:2154`. Printed at `pm_gbi.c:2155` as `pmu_ic pmu_dc pmu_cyc`.
- `3ds/include/pm_port.h` has no PMU enum. No libctru header is on disk. `C:\devkitPro` and `/c/devkitPro` do not exist. The numeric event IDs cannot be checked against `svc.h`.

## Event IDs

| Window | Log ev | Code name (pm_phase.c) | Counter | Role |
|---|---|---|---|---|
| A | 0 | PERFCOUNTEREVT_CORE_INST_CACHE_MISS | PMN0 | I-cache miss count |
| A | 11 | PERFCOUNTEREVT_CORE_DATA_CACHE_READ_MISS | PMN1 | D-cache read miss count |
| B | 16 | PERFCOUNTEREVT_CORE_MAIN_TLB_MISS | PMN0 | main TLB miss count |
| B | 1 | PERFCOUNTEREVT_CORE_STALL_BY_LACK_OF_INST | PMN1 | instruction-fetch stall count |

The name-to-number link comes from the log line, which prints the enum value. The numbers 0, 1, 11, 16 are not checked against a header. The ARM11 PMU spec is not on disk.

## sp0c field mapping

- Window A: `pmu_ic` = I-cache misses per frame, `pmu_dc` = D-cache read misses per frame.
- Window B: `pmu_ic` = main TLB misses per frame, `pmu_dc` = instruction-fetch stall cycles per frame (event count).
- `pmu_cyc` = cycles per frame, both windows.
- The sp0c line does not print the window. Window A ends at a 300-frame boundary where `f/300` is odd. Window B ends where `f/300` is even. This assumes the sp0c frame counter matches `pmu_f`.
- Valid for a cache share: `pmu_ic` and `pmu_dc` in window A only, and `pmu_cyc` in either window.
- Not valid as a cache share: `pmu_ic` in window B (TLB), `pmu_dc` in window B (stall).

## Still window values (log: build3ds/hw_live_log_170.txt)

Window B (sp0c f5317, f5917; pmu f5400, f6000 ev 16 1):
- f5317: pmu_dc 6053311, pmu_cyc 14175937, ratio 0.4270
- f5917: pmu_dc 6031406, pmu_cyc 14123768, ratio 0.4270

Window A (sp0c f5617; pmu f5100, f5700 ev 0 11):
- f5617: pmu_ic 151332, pmu_dc 54493, pmu_cyc 14095541
- pmu f5700: 150863, 54478, 14076173

## Cache share formulas

- Instruction-fetch stall share: `stall_per_frame / cyc_per_frame` = pmu_dc(B) / pmu_cyc(B) = 0.427 (42.7 percent of cycles).
- I-cache miss rate: `pmu_ic(A) / pmu_cyc(A)` = 151332 / 14095541 = 0.0107 misses per cycle.
- D-cache read miss rate: `pmu_dc(A) / pmu_cyc(A)` = 54493 / 14095541 = 0.00387 misses per cycle.
- Cycle share of cache misses: `(I_miss x P_I + D_miss x P_D) / cyc`. P_I and P_D are the miss penalties in cycles. Neither is measured by this set. The share cannot be computed from these events.

## Uncertainty

- The numeric IDs (0, 1, 11, 16) are not checked against the libctru header or the ARM11 spec.
- A and B are different 300-frame windows. Stall and miss counts never cover the same frames. The formulas assume the scene is steady across the window pair.
- STALL_BY_LACK_OF_INST is not a pure I-cache event. It can include TLB and branch effects. Treat 0.427 as an upper bound on the instruction-fetch share, not an exact value.
- 14.1M cycles per frame is high for a 268 MHz ARM11 at 60 fps (about 4.5M per frame). The unit or the frame rate is unclear. Check the console clock before you use absolute cycle counts.
- The counters measure the core that runs the svc call. Which core runs the gbi thread is not checked here (see inv_core_assignment.md).
