# Row DRECSTEP: the drec switch takes a step (`drec=<first>:<count>:<step>`), so one run records one frame per fxtest slot

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main of 2026-11-13)
- 3ds/src/pm_gpu.c drec_begin (:595-608): `drec=<first>:<count>` is parsed at :598 with strtoul. drec_on (:601) is true for
  first <= fr < first + count. drec_end (:610-616) closes the file when `pm_frame_count() + 1u < drec_first + drec_count` is false
  and logs `drec f%u draws %u bytes %ld`.
- fxtest runs the effects of `fxtest=a,b,...` one per 44 frames from frame 400 (pm_fxtest.c:179-180). A batch row records frame
  412 + 44 i for effect i.
- Both functions are cold (once per frame, not the per-draw path), so one `%` per frame is fine (hw_guidelines.md: no division
  in hot loops).
Start: `sh 3ds/tools/wt_new.sh DRECSTEP`. PM_ROW=DRECSTEP PM_LANE=DRECSTEP on every call.
## Touch only (places)
3ds/src/pm_gpu.c drec_begin and drec_end only; new 3ds/tools/azt.d/DRECSTEP.json and bench.d/DRECSTEP.json.
New names: drecstep_89_33. Host link: none (pm_gpu.c is not in a host link).
## Exact edits
1. `static unsigned drec_step = 1u;` next to drec_first. At :598: parse a third field. `char* e2 = e; drec_count = 1u;`
   `if (*e == ':') { drec_count = (unsigned)strtoul(e + 1, &e2, 10); if (*e2 == ':') drec_step = (unsigned)strtoul(e2 + 1, NULL, 10); }`
   `if (!drec_step) drec_step = 1u;`
2. :601: `drec_on = drec_win > 0 && fr >= drec_first && fr <= drec_first + (drec_count - 1u) * drec_step && (fr - drec_first) % drec_step == 0u;`
2b. Frame lines (step > 1 only, so the old drec.txt form stays for every scene with step 1): right after the drec_on line,
   `if (drec_on && drec_f && drec_step > 1u) fprintf(drec_f, "frame %u\n", fr);`, and after `drec_seq = drec_over = 0;` at the end
   of drec_begin, `if (drec_step > 1u) fprintf(drec_f, "frame %u\n", fr);`. A per-file diff of a batch reads these lines.
3. drec_end: the close test becomes `if (!drec_f || pm_frame_count() < drec_first + (drec_count - 1u) * drec_step) return;`
   (with step 1 this is the old test).
4. azt.d/DRECSTEP.json drecstep_89_33: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=89,33 loglevel=debug det drec=412:2:44`, until `drec f456 draws [1-9]\d* bytes \d+`, cap
   40, deadline 35. bench.d/DRECSTEP.json: drecstep_89_33, kind azahar_pic, tags azahar det drec.
5. Commit: `DRECSTEP: drec=<first>:<count>:<step> records one frame per step`.
Correction 1 (ORACLE8, 2026-11-13; resume after 4d6727c95, budget reset once): azt.d/DRECSTEP.json takes first_marker
`warp: mac_01 entry 0` and the warp event in place of `prof=1 log=off` and the prof event (pm_log.c:655-658 prints that only with
prof=). Fix the json, commit, then the seed step again.
## Gate (expected values)
1. Build, memcheck OK. nd_host PASS.
2. No change at step 1: `bench.py base spot_mac01_drec` (merge-base), run it on the row, cmp: drec.txt byte-equal.
3. `bench.py run drecstep_89_33 --row DRECSTEP --why "DRECSTEP window"`: the log has `drec f456 draws N bytes B` with N above 0.
   drec.txt has exactly 2 lines `frame 412` and `frame 456`. Hand back N and B.
4. Seeded fault first (a value change, then undo): in edit 3 `drec_count * drec_step` in place of `(drec_count - 1u) * drec_step`:
   the log then says `drec f500`, and the until does not match within the cap (the gate fails as expected).
5. Budget 8 Azahar starts. Counter: none moves (tool row for the codemod batch rows).
## Stop and end
A drec difference at step 1, a window line other than f456, or a tool fault: stop, commit, hand back the exact output.
Delete this row file at the end.
