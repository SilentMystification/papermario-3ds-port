# Row ESCAPE: START + SELECT leaves a hung game (debug)

## User facts (2026-11-10, O3DS XL)
The 3D slider hang needs a hard restart (power button). The user asks for a debug toggle: START + SELECT held at the same time
returns to the HOME Menu, usable while the game is hung.

## Facts (orchestrator)
- The watchdog (3ds/src/pm_wdog.c) already sees a stall after 10 s and calls svcExitProcess (:45). After that the console still
  needs a hard restart, so a plain process exit does not free the system when the GPU or GSP is stuck.
- The game thread is the one that hangs. The escape must run on a thread that keeps running (the watchdog thread) and read the
  buttons without the game's input code (the HID shared memory, not hidScanInput of the game thread).

## Oracle task
Design the smallest escape: which thread polls START + SELECT, how it reads the pad, and what it calls so the console returns to the
HOME Menu with no hard restart while the GPU is stuck (for example release the GPU right, then exit; or a system reboot call). Say
what is safe while another thread holds the GPU. Debug only (on in debug builds or by a debug switch). Give the Haiku row and its gate.
