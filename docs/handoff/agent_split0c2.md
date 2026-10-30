# agent_split0c2 (row SPLIT0C2)

State: diagnostic only. Flush time inside the sp0c tex kind is timed and printed. No draw change. One Azahar start (1 of 3).

Code (3ds/src/pm_gbi.c):
- sp0_F: flush ticks inside the tex kind. sp0_in: 1 between sp0_group_begin and sp0_group_end.
- Two timing sites, both call pm_gpu_flush (macro, __LINE__ kept at the call site): tb_run SETOTHERMODE_H filter change (about 2870), tex_op G_SETTILE tile 0 change (about 2948). Each site reads pm_ticks only when pm_ph_on, sp0_in and sp0_cur == 1.
- pm_gpu.c not edited. flush_impl and flush_draw (pm_gpu.c 557 to 567, 751) run inside the timed call.
- sp0c line: `sp0c f<n> tex <us> dlc <us> rest <us> flush <us> skip <n> grp <n>`. flush is a subset of tex. tex_op = tex - flush.

Azahar result (mac01t_cap1, 1 start): `sp0c f483 tex 1912 dlc 902 rest 970 flush 406 skip 27773 grp 34513`. Top shot 0000_1_2 (frame 500): Mario and a Toad, HUD on.
- Caveat: tick reads cost about 1.3 us each in Azahar. The flush sites add 2 reads per flush. tex rose from 1714 (SPLIT0C run) to 1912, a different run. Not a like-for-like delta.
- Flush counts per frame are not printed.

Next steps (orchestrator decides):
- If flush is a large share on console, the next row works on batch close (plan_dl_design.md), not on the bind.
- Console push with phase (rule 6), after the epic check.

Commands:
- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then restore the user map file (copy kept outside the repo before gen).
- Test: `PM_ROW=SPLIT0C2 powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row SPLIT0C2 -Try`
