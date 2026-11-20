# ws_NATEF: the entity fallback run goes; an entity with no cut table is refused; ent_run leaves the elfgate count
READY. Start commit: main. Oracle NATIVE12 (id in the launch prompt). Worktree build3ds/wt/NATEF, branch NATEF. PM_ROW=NATEF PM_LANE=NATEF. WAIT: none. Model: the brief names Sonnet. The edits are closed, so Haiku can do them too.
Regions (main 850e27316e, find each line by its text): nd_ent.c :331, :347, :405-406, :412-420, :431-432, :444; nd_walk.c :219, :431-443, :524; nd_int.h:319. No overlap with NATRS, NAT6A-C (their regions: nd_walk.c :250-254, after :383, after :394, :402, :546-555).
Counters: C13 c0gate calls -2 (FACT: nd_ent.c:417 calls nd_ent_units, nd_walk.c:438 calls nd_nr_exec inside it). Interp -1 (FACT: FUNC nd_ent_units in basebin 99b756502668c647). State 0. FACT main e5ae433d74: 80 / 32 / 23, so 78 / 31 / 23 if no other row merges first. elfgate: func 1 to 0 (FACT: func ent_run 0x0013bacc, pattern `ent_run` in elfgate_patterns.txt:1).
## Facts (the proof)
- FACT ent_run (nd_ent.c:413-417) runs k->fw units on the interpreter only when et is NULL or a bound has no cut. et comes from nd_walk_ent :183 (NDC_ENT), nr_call :219 (ab[3], NULL for a CALL unit with no bit 2), gt_run0 :524 (rec[5] or rec[9]).
- FACT the `entfb ... why 0` lines of the NAT5A gate (spot_mac01 and fxparts f35, entcape_kmr03 f39-f40) came from the gt_run0 debug branch: on a pm_walk_dbg frame it ran `{ND_OP_CALL_W0, key}` through nd_nr_exec, so nr_call got ab NULL. NAT5T 3a (0fac0ec8c4) deleted it. FACT spot_mac01_drec on ebc6ef28f (runs.jsonl 02:43) logs 0 entfb lines.
- FACT the entfb list address is an NdNDev key (ndd_ entry, list = own address). The run ELF is gone, so a symbol name from another build is not valid.
- Paths left in code (GUESS: not hit in the 4 drec benches): static nested calls under a refused NDC_ENT (ent_dl_walk, then nd_ntab_run), a callee with no NdNDev, depth 12, count mismatch (why 1). The new src field names the caller if one fires.
## Edits (Edit tool only: CRLF files. New or changed lines end with `/* NATEF */`)
1. nd_ent.c: replace :412-417 (ent_fb_note and ent_run) with:
```c
unsigned char nd_ent_src; /* NATEF: the caller of nd_eg_run for the entfb line: 1 nr_call static, 2 nr_call copy, 3 gt_run0, 4 ent_draw */
static __attribute__((cold, noinline)) void ent_fb_note(const EntKey* k, unsigned why) { static unsigned c[3]; if (why > 2u || c[why] >= 3u) return; c[why]++; pm_log_evt("entfb f%u l %08x why %u nops %u src %u", pm_frame_count(), (unsigned)k->list, why, (unsigned)k->nops, (unsigned)nd_ent_src); } /* NATEF: src */
static void ent_cut_run(const EntKey* k, const uint32_t* et, u32 a, u32 b) { /* NAT5A; NATEF: the cut slice only, et is never NULL here (nd_eg_run) */
    nd_ent_gbi++;
    if (et[1u + a] && et[1u + b]) { const uint32_t ab[2] = { et[1u + a], et[1u + b] }; ent_cf[0]++; nd_ent_slice(ab); return; }
    ent_cf[1]++; ent_fb_note(k, 2u); } /* NATEF: a bound with no cut (none by the NAT5A cut rule): the piece is not drawn, one log entry */
```
2. nd_ent.c :420, :431, :432: `ent_run(` becomes `ent_cut_run(`. Example :420. before: `    ent_run(k, et, a, b); /* DEL1B; NAT5A */` after: `    ent_cut_run(k, et, a, b); /* DEL1B; NAT5A; NATEF */`. In the :418 comment replace `end of nd_ent_units` with `end of nd_ent_slice`.
3. nd_ent.c:444. before: `    if (et && et[0] != k->nops) { ent_fb_note(k, 1u); et = NULL; } /* NAT5A */` after: `    if (!et || et[0] != k->nops) { ent_fb_note(k, et ? 1u : 0u); ent_c.fb++; return 0; } /* NAT5A; NATEF: no cut table or another count: the caller runs the list */`
4. nd_ent.c:406. before: `    return nd_eg_run(l, k, rec[1].words.w0 | (rec[2].words.w0 << 8), et);` after: 2 lines `    nd_ent_src = 4u; /* NATEF */` and the old line. :331 comment `/* NAT5A: copy runs, fw runs */` becomes `/* NAT5A: copy runs; NATEF: pieces with no cut */`. :347 `copy %u fw %u` becomes `copy %u nocut %u`.
5. nd_walk.c: delete :431-442 (the 2 comment lines `/* DEL1B: n flat units ...` and the whole nd_ent_units function). :443 before: `/* NAT5A: as nd_ent_units, the units of a cut raw alt range ab (nd_alt_eput). */` after: `/* NAT5A: the units of a cut raw alt range ab (nd_alt_eput), with the list start and end of nd_nr_list (keep 1). NATEF */`
6. nd_walk.c:219: after `pm_tex_tagi = 5; ` insert `nd_ent_src = ab ? 2u : 1u; /* NATEF */ `. nd_walk.c:524: insert the line `        nd_ent_src = 3u; /* NATEF */` before `        const int eh = ent_gdl(next, ab ? ...`.
7. nd_int.h:319: replace the nd_ent_units prototype with `extern unsigned char nd_ent_src; /* NATEF, nd_ent.c: the caller of nd_eg_run (entfb line) */`. Keep `interp nd_ent_units` in c0gate_names.txt.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only; 1 build, at most 8 Azahar starts)
1. Edits 1-7. Commit `NATEF 1: entity fallback run deleted; no cut table refuses the draw; ent_run renamed`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec > build3ds/out/natef_base.txt 2>&1`, then the same for fxparts_drec (append `>>`). The output names the basebin entry of the base.
3. `sh 3ds/tools/rowloop.sh --row NATEF --bench entcape_kmr03_drec,fxparts_drec --why "NATEF: entity fallback run deleted" > build3ds/out/natef_loop.txt 2>&1`.
4. `grep -H "entfb\|entcopy" build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/natef_fb.txt`.
5. One script into build3ds/out/natef_c0.txt: `python 3ds/tools/c0gate.py build3ds/lane-NATEF/pm_3ds.elf`; `python 3ds/tools/elfgate.py build3ds/lane-NATEF/pm_3ds.elf`; `git archive $(git merge-base HEAD main)` extracted to build3ds/out/c0base, then `python build3ds/out/c0base/3ds/tools/c0gate.py <main folder>/build3ds/basebin/<entry of step 2>/pm_3ds.elf` (the base line).
## Gate (exact): LOOP PASS (host PASS, build OK, 2 drec byte-equal). c0gate head minus base: calls -2, interp -1, state 0, and the head interp_syms line has no nd_ent_units. elfgate head: `func 0 data 0 gfxnames 0`. natef_fb.txt: report the lines, not judged (expected: no entfb line).
Fix rights: compile errors, unused-variable warnings, the count in 3ds/tests/lkey/allow.txt. A DIFF, a crash, other c0gate or elfgate values, or a tool fault: stop. SendMessage natef_fb.txt, the cmp path and the first DIFF line to oracle NATIVE12. Wait for the reply.
## Hand-back: last commit `NATEF 2: gate. <LOOP lines> <c0gate lines> <elfgate line>` with `git add -f build3ds/out/natef_loop.txt build3ds/out/natef_fb.txt build3ds/out/natef_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, entfb lines, c0gate and elfgate lines.
