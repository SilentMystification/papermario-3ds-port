## DC6 (2026-10-29): dcsum cmd words and group counts per close

| Scenario | What it proves | Command | First marker | Status |
|---|---|---|---|---|
| mac01t_dcsum | dcsum line (cmd words per sampled close) and dcgrp line (groups per close) on the mac_01 tour view; shots at frames 500 and 504 | `azt.ps1 mac01t_dcsum -Row DC6 -Build dc6 -Try` | `warp: mac_00 entry 4` | unverified (3 runs for the log; top shots 2 of 2 byte-equal to HEAD fb9e8f0fc, picture read: Mario and a Toad on the Toad Town street, HUD on) |

- HEAD (fb9e8f0fc): cmd field reads 0 permil. The citro3d usage (C3D_GetCmdBufUsage) updates only at a split, so the delta was always 0. DC6 reads GPUCMD_GetBuffer offset instead: 80 to 82 words per sampled close (dcsum f299).
- dcgrp f299 (hundredths per close, Azahar): shadow miss 168, bind0 101, bind1 0, bindstaged 1, tev 62, palup 53, palsame 0, pmul 33, prog 12, bufinfo 12.
- palsame 0: no palette upload writes the matrix the register already holds. Shadow misses and binds are real value changes (DR1 and the TEV key already compare).
- Draw stage 14 us per close (Azahar) on both HEAD and DC6. No repeated group remains to remove; no draw change in DC6.
