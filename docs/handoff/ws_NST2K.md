# ws_NST2K (Haiku): dschk names the runner of each quiet native command (key 2090a); measure only (stage S3; counter: epbad keys by runner)
Designer: oracle FIN. WAIT: branch from main after the NST2J merge (both edit nd_rec.c and nd_dschk.c; NST2J moves the line numbers below). Worktree build3ds/wt/NST2K, branch NST2K.
PM_ROW=NST2K PM_LANE=NST2K. Edit only: 3ds/src/nd_rec.c (2 places), 3ds/src/nd_dschk.c (2 places), 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main a6cd9f0033; find each line by its text after NST2J)
- NST2I gate (FACT, dschk_base.txt): every epbad key of the 4 scenes is 2090a (side 2, rule 9, op NDC_NAPI = 10, nd_cmd.h:20), e.g. fxchk_a 2090a=16202.
  Rule 9 (nd_rec.c rec_take) marks every command as a cut; a 2090a key is a native command whose core 0 runner moves no gv_epoch.
- The key holds only the op (nd_cmd_last, nd_rec.c:180, set in stream_run :673). About 20 runners share NDC_NAPI (fills, texture rectangles, HUD,
  glyphs, sprites, lists, parts, targets, transitions); some are static functions in src/ files, so the key must hold the runner address.
- Keys of the other forms stay below 0x30000; a runner address is above 0x00100000 (.text). ep_note keeps 32 keys (nd_dschk.c ep_note).
## Edits (dschk only: drec stays byte-equal)
1. nd_rec.c after the nd_cmd_last line (:180): `uint32_t nd_cmd_run; /* NST2K: the runner of the last NDC_NAPI command core 0 ran, else 0 */`.
2. nd_rec.c stream_run, the NST2I line (:673). Before: `... nd_dschk_run))) nd_cmd_last = (uint8_t)op; } /* NST2I */`
   After: `... nd_dschk_run))) { nd_cmd_last = (uint8_t)op; nd_cmd_run = op == NDC_NAPI ? p[1] : 0u; } } /* NST2I, NST2K */`
3. nd_dschk.c after the nd_cmd_last extern (:17): `extern uint32_t nd_cmd_run; /* nd_rec.c: the runner of the last native command (NST2K) */`.
4. nd_dschk.c, the side 2 line (:145). Before: `            else ep_note(0x20000u | (((epw >> 16) & 0xffu) == 9u ? (0x900u | nd_cmd_last) : ((epw >> 8) & 0xffffu)));`
   After: `            else ep_note(((epw >> 16) & 0xffu) == 9u && nd_cmd_run ? nd_cmd_run : (0x20000u | (((epw >> 16) & 0xffu) == 9u ? (0x900u | nd_cmd_last) : ((epw >> 8) & 0xffffu)))); /* NST2K: a quiet native command keys by its runner address */`
   In the ep_note comment add: `a key above 0x00100000 is the runner address of a quiet native command (NST2K)`.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NST2K 1: dschk keys a quiet native command by its runner`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2K --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes (azt.d/NST3.json); `grep -a "dschk"` > build3ds/out/nst2k_dschk.txt; replace dschk_base.txt with it.
   Pass: bad 0; cmds and epbad equal to the NST2J lines of dschk_base.txt; no key 2090a left; the epsrc counts plus `other` sum to epbad.
4. Name each address key: `MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)/build3ds/lane-NST2K:/e:ro" devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -e /e/pm_3ds.elf 0x<key> ...`
   > build3ds/out/nst2k_names.txt (the ELF of the gate build; FACT: decode_dump.py:36-39 runs the same command). Commit the file.
Fix rights: the listed files; compile errors; the docker path of step 4. A DIFF, a crash, an epbad change: stop, send the output path to the oracle. Wait.
Stop: last commit `NST2K 2: gate. dschk bad <4> epbad <4>; runners <name=count, top 4 per scene>` deletes this page. One SubagentHandback with nst2k_names.txt and the epsrc lines.
