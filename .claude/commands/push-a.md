---
description: Build and run in Azahar, live log
---

Run this exact command via Bash, in the background:

```
powershell -NoProfile -File 3ds/go.ps1 push a -Timeout 60 $ARGUMENTS
```

Do not re-derive the command, do not ask for confirmation, do not explain what the script does -
just run it. This force-kills any Azahar window already open - if the user mentioned having their
own session open with progress, confirm before running. When it completes, report the outcome in a
few lines. Full detail is in `build3ds/last_log.txt`, `build3ds/last_shot.png` (if `-Capture` was
in $ARGUMENTS), and `build3ds/last_exception.txt` on a crash.
