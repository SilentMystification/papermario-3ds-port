---
description: Build and push to O3DS hardware (192.168.1.168), live log
---

Run this exact command via Bash, in the background (do not wait inline, this touches real hardware):

```
powershell -NoProfile -File 3ds/go.ps1 push o -Seconds 75 $ARGUMENTS
```

Do not re-derive the command, do not ask for confirmation, do not explain what the script does -
just run it. When it completes, report the outcome in a few lines (build result, push result, any
`[CRASH]`/exception lines, final fps/stutter numbers if present). Full detail is in
`build3ds/hw_live_log_168.txt` and `build3ds/last_log.txt` if more is needed.
