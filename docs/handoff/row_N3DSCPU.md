# Row N3DSCPU: no APT CPU limit change at HOME/sleep on the N3DS (user, 2026-11-09)

## User facts
N3DS, main c1b0e44e6 (with the HOMEHANG 200 ms bound): "still crashing when pressing the home button. I think that it's related to
changing the apt % on the n3ds. We shouldn't change it there." Live log: watchdog after 10 s, game thread phase 10, steps 462;
no "home: quiesce" line printed at all (the log writer runs on core 1).

## Code (orchestrator)
pm_log.c: start sets the APT app CPU limit to 80 (cpu_apply, :1244); quiet_enter (HOME suspend / sleep, about :1140) sets 30;
resume and wake set the game value back (:1187, :1213); exit sets 30 (:1216, :1223). Core 1 threads (log writer, audio, prefetch,
flash) depend on that limit.

## Start state
`sh 3ds/tools/wt_new.sh N3DSCPU`, branch N3DSCPU. PM_ROW=N3DSCPU, PM_LANE=N3DSCPU. One coder agent (the only agent running).

## Change
On the N3DS (APT_CheckNew3DS or the existing core2 probe), do not call cpu_apply at HOME suspend, sleep, resume, wake or exit: keep
the start value. Log one line at the hook: "home: cpu limit kept (N3DS)". The O3DS keeps today's behaviour. Keep the HOMEHANG bound.

## Gates
nd_host PASS, build OK, memcheck OK. Console (user): N3DS HOME in a field and in a battle, return, wait 30 s; O3DS XL HOME once.
Commit; delete this row file. No Azahar start (Azahar has no HOME menu).

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls.
