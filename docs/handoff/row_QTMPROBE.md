# Row QTMPROBE: can an N3DS CIA map, write and read the QTM RAM region (probe, do not start)

(Model: Haiku. Feasibility probe for the N3DS QTM RAM note in row_ORACLE_NEXT.md. BLOCKED: needs a CIA build (RSF + makerom), which
the repo does not have; that build is its own row, written when the user asks for CIA output.)

## Facts (libctru os.h)
OS_QTMRAM_VADDR 0x1E800000, OS_QTMRAM_PADDR 0x1F000000, OS_QTMRAM_SIZE 0x400000; the last 128 KiB is kernel-reserved. Unverified
claim: a static memory mapping descriptor in the ExHeader ARM11 kernel capabilities maps it for the title.

## Start (after the CIA build row is merged)
`sh 3ds/tools/wt_new.sh QTMPROBE`. PM_ROW=QTMPROBE PM_LANE=QTMPROBE.

## Touch only
The CIA RSF (the ARM11 kernel capability list: one static mapping descriptor for 0x1F000000 .. 0x1F3E0000), a new cold function
`pm_qtm_probe()` in 3ds/src/pm_mem.c called once at boot under the debug switch `qtmprobe`.

## Exact edits
1. RSF: add the mapping descriptor for the physical range 0x1F000000 - 0x1F3E0000 (3.875 MiB) read-write, as the libctru os.h
   addresses name it; keep every other capability.
2. pm_qtm_probe (switch qtmprobe): `if (!APT_CheckNew3DS)` log "qtmprobe: o3ds, skipped" and return. Query the QTM service state
   (qtmCheckInitialized or the service handle) and log it. Then, at OS_QTMRAM_VADDR: write a 32-bit pattern (0x5A5A0000 | i) at the
   start, the middle and the last word before the reserved 128 KiB, flush the data cache for those lines, read back, and log
   `qtmprobe: map ok %d write ok %d/%d qtm %d` with the three compares. A data abort is the answer "not mapped": the crash handler
   (pm_crash_page) prints the address; record that line.
3. A GPU read check: copy a 64x64 RGBA8 test texture into the region, bind it with C3D_TexInit pointing there (if C3D refuses a
   non-linear address, log the refusal), draw one quad to an off-screen target, read 4 pixels back, log them.

## Gate (probe; console, N3DS 192.168.1.224, one push of the CIA with qtmprobe)
The qtmprobe lines as printed (or the crash line). No Azahar gate (Azahar has no QTM RAM model). Hand back the lines.

## At the end
Delete this row file in the completing commit.
