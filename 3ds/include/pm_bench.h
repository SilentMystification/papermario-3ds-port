#ifndef PM_BENCH_H
#define PM_BENCH_H

/* S0 BENCH: micro-benchmarks for the O3DS redesign (docs/plans/arch_review.md part 3, docs/plans/bench_notes.md).
 * Switch `bench` runs all, `bench=<list>` runs the named benches (draw, drawpm, r2, flush, svcflush, tex, replay, fbwait).
 * Switch `benchexit` leaves the frame loop after the benches. Result lines start with `bench:`.
 * Call once at the start of pm_frame_loop. Returns 1 when the caller must leave the loop (benchexit), else 0. */
int pm_bench_run(void);

#endif
