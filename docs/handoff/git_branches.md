# Git branches: recovery record (row GIT, 2026-11-06)

Tip hashes of every local branch before cleanup. Recover a deleted branch with `git branch <name> <hash>` (objects stay until gc).

| Branch | Tip | Verdict | Action |
|---|---|---|---|
| hang1a-cap | 9a0e4b87762a54a0e0cdf4c5d05b5fdd97609070 | MERGED (git cherry: patch in nds1) | deleted |
| menu5-untested | e01dcc15e15f8d6780f7664e5fd721efbac16680 | MERGED (nds1 a84284034 "Merged from menu5-untested", rewritten) | deleted |
| native | 5027dcf16a175f60ce1eec419394670ab8ac64a9 | MERGED (ancestor) | deleted |
| prof4 | 5027dcf16a175f60ce1eec419394670ab8ac64a9 | MERGED (ancestor) | deleted |
| pushbuild | dfdbdbc3aba790dc07842e56b3eb30ad5f9d3251 | MERGED (only merge commits of native and flk1) | deleted |
| menu | 2683798a2432e25bb9d007cd490f897a2ec58c83 | MERGED (ancestor) | deleted, worktree removed |
| s4 | e1bc84153287c5dabb96d099414887ec724e74ca | MERGED (ancestor) | deleted, worktree removed |
| worktree-agent-a9be3c33b63e9fd70 | f56fc77ada89c3cf12adc0e157e2b7cdd4fb916b | MERGED (ancestor) | deleted |
| worktree-agent-aa6f29dfae0e09271 | f56fc77ada89c3cf12adc0e157e2b7cdd4fb916b | MERGED (ancestor) | deleted |
| tex1-clamp | e8896aa978164f4690f9977496b8cf1068094149 | STALE (TEX1 result: change does not alter the terrain, not merged) | deleted |
| texl2 | 77fd90122e7fee3cd761e0426d44616f1e4cc250 | STALE (TEXL2 dropped by user decision) | deleted |
| texl2-l2c-wip | b55f8fee93311b3292c670f13c4591776fdb4351 | STALE (TEXL2 dropped, failed gate) | deleted |
| uc2-percall | dc760f5c22b0ab63905a90bb989b89f21678806c | STALE (UC2 solved on other branches; conflicts in pm_gbi.c, pm_gpu.c, pm_nusys.c; stereo is in nds1) | deleted |
| gkcam | 61d04a54191cf0a7915939ed5a63ad84420af402 | OPEN (scenarios and handoff of an unreproduced symptom) | kept |
| gkcam-azt | 3f85f0c40bad0e18d5a67be4f0f41c22d9e72f53 | OPEN (scenarios and catalog rows) | kept |
| stereor | 4e5fbb456a2f637752b597c65a64950187867f5e | OPEN (STEREOR scenarios, row INCOMPLETE) | kept |
| txtflick | f9e0868c890c0440d5b462d7d6911af36d6f485d | OPEN (TXTFLICK probe and scenarios, row INCOMPLETE) | kept |
| texwin-fix | afbcfd4d34f5a970510912bc367a52733f2f5977 | OPEN (msgbox_rule and fxhit_rule scenarios, TEXWIN results) | kept |
| hud1 | ddc31e84f5260f6b08add299d4168a46dae0ccdc | locked worktree of another session | untouched |
| worktree-agent-ada36f387b16d4402 | f56fc77ada89c3cf12adc0e157e2b7cdd4fb916b | belongs to the locked hud1 worktree | untouched |
| main | 40408f05f3882ad1f23d582c2791f99e80d4bd02 | ancestor of nds1 | moved to nds1 |

No NEAR branch: no merge was needed. The OPEN branches hold only azt scenarios and docs; they conflict with nds1 in azt_scenarios.json and test_catalog.md (append conflicts).
