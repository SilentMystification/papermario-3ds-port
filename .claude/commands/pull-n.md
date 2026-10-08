---
description: FTP-pull game log and Luma crash dumps from N3DS hardware
---

Run this exact command via Bash, in the background (do not wait inline, this touches real hardware):

```
powershell -NoProfile -File 3ds/go.ps1 pull n $ARGUMENTS
```

Do not re-derive the command, do not ask for confirmation - just run it. One attempt, short
timeout; if the console isn't reachable it fails in ~8s, report that plainly rather than retrying.
Output lands in `build3ds/hw_n3ds_log.txt` and `build3ds/hw_n3ds_dumps/`. Summarize what was pulled
(log size, any crash dumps found) in a few lines.
