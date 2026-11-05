## LOGCON (console log level, separate from the file and live log level)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| logcon_err | `conlevel=error` with `loglevel=debug`: log.txt holds debug lines (`logstat frames 300 work`), the logstat2 line prints `console lines 0` (gate max 0). Until is the logstat2 line at frame 300. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcon_err -Row LOGCON -Build logcon -Try` | `warp: kmr_20 entry 4` | 20 s to the first marker, about 60 s to the until | build3ds\azt\shots\logcon_err | Run 1 (build 1670-logcon): 3 of 3 log checks. logstat2 `console lines 0`; log.txt has the `logstat frames 300` debug line. Live socket log not checked (log.txt only). Picture not read by the agent: the bottom screen must show no log text. |
| logcon_dbg | Same build with `conlevel=debug`: the logstat2 line prints `console lines` at least 1 (gate min 1). Second start of the pair. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 logcon_dbg -Row LOGCON -Build logcon -Try` | `warp: kmr_20 entry 4` | as logcon_err | build3ds\azt\shots\logcon_dbg | Run 2 (build 1670-logcon): 3 of 3 log checks. logstat2 `console lines 105` (was 0 in logcon_err). Picture not read by the agent. |

