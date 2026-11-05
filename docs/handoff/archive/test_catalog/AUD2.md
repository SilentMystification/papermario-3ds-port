## AUD2 (2026-11-03): sound start retry and num_pvoice log (lane native)

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| aud2_cache (unverified, audio log only) | Yard with `dspcache=256`: sound starts that get no cache entry are retried on the next block (up to 16 blocks, dsp_voice.c AU2_START_RETRY). `audio dsp` line prints cache hits, misses, evictions and failures; the `dspaudio: num_pvoice 24` warning prints once. Run 1: failures 122 (attempts), starts 230 (successful), evictions 31, sample key 00fd6090 retried on consecutive blocks. | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 aud2_cache -Row AUD2 -Build native -Try` | `warp: kmr_20 entry 4` | about 16 s, cap 25 s | none (no picture) | The log does not show whether each retry succeeded. Failures count attempts, not dropped sounds. |

