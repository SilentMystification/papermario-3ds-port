## OPTFIX (2026-11-03): PmOpt initializer, boot option values

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| boot_opts (unverified) | Boot with no options.ini and no options.txt: the `options: boot` line prints dz_cs 0, rep_delay 500, rep_rate 200, hud_ms 1000, conlevel 1, loglevel 1, log 1 (the defaults). Log only. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 boot_opts -Row OPTFIX -Build OPTFIX -Try` | `boot_main` | cap 30 s, until at the boot line | none | The sdmc options.ini and options.txt must be moved aside for the run. The debug string loglevel=debug lets the info line print. |

