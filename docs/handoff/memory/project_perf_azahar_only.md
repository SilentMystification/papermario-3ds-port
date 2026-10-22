---
name: project-perf-azahar-only
description: Perf work runs in Azahar only for now; O3DS push failed (flaky Wi-Fi); MA1 logic sites and speed measurements are saved for hardware runs
metadata:
  node_type: memory
  type: project
  originSessionId: ac571f0b-1513-4085-9778-3f58937ad7cd
  modified: 2026-10-20T06:24:35.500Z
---

As of 2026-10-20 the user does not test on the O3DS for a long time. The baseline is the Azahar full intro+demo cycle (3ds/logs/perf/baseline_azahar_840.txt, O3DS config, fast mode, cpulimit=80). The O3DS console Wi-Fi is flaky, netloader (port 17491) often fails.

**Why:** the user cannot reach the console reliably. Azahar gives exact counters. User correction 2026-10-20: Azahar timing also shows the right direction of a speed change (not exact). Use emulated fps of the full cycle as the speed metric and target higher fps; keep the limiter off or a cap that does not limit.

**How to apply:**
- Do not ask the user for O3DS runs or numbers. Do not block rows on them.
- Q-MA1-D8, D9, D10, D11-D12, P16 (game-logic math edits) are parked for hardware runs. Do not ask again until the user brings the O3DS up.
- PO3b gate ("O3DS GBI share after PO2") cannot be met; the user decides on Azahar evidence.
- Pushes to the console are done by the orchestrator session, not by an agent, so the user can see them (see [[feedback-visible-status]]).

2026-10-21 user decisions: the user gave an explicit go to trust Azahar numbers as a reference point (direction and work counts; not exact). Audio moves to the DSP (ndsp hardware voices, one-time offline conversion accepted). Per-triangle CPU work and the hot-path accuracy hacks and the draw cost (66 us per draw) are to be optimized (renderer redesign, see docs/plans/arch_review.md when it exists).
