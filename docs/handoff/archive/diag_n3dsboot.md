# Diagnosis: N3DS (New Nintendo 3DS, 192.168.1.224) live log gives 0 lines

Row N3DSBOOT. Source read only. Branch uc2c, base commit 761ca5532. Code read at HEAD.
Files: 3ds/src/pm_log.c, 3ds/src/pm_main.c, 3ds/src/pm_options.c, 3ds/include/pm_port.h, 3ds/src/pm_warp.c (pm_debug_value), 3ds/src/pm_input.c, 3ds/push_3ds.sh, 3ds/tools/livelog.py.

## Symptom

TCP (Transmission Control Protocol) port 17492 accepts. livelog.py prints "[livelog] connected". No line arrives for 60 to 120 s. O3DS (original 3DS, 192.168.1.131) with the same build returns 616 lines.

## Why "connected" with no lines

`listen(listen_fd, 1)` in pm_log_init (pm_log.c:266-270) completes the TCP handshake from the kernel backlog. The client sees "connected" before the game calls `accept()`. So "connected" only proves that pm_log_init ran. It does not prove the writer accepted the client.

`accept()` runs in exactly two places:
- writer_main, pm_log.c:625-627: `if (log_file_on) { accept_client(); read_cmd(); }`. Runs only when the log gate is on and pm_io_quiet is 0 (pm_log.c:623-624).
- pm_log_poll, called from the frame loop (pm_main.c:373): `if (!writer_on) accept_client();`. Runs only when the writer is not started.

Every line goes to the socket only from out_batch, which runs only from the writer (pm_log.c:~517 `send(client, ...)`). With no accepted client, no line is sent.

## Hypotheses, ranked

### H1. Log gate off from options.ini log=0 (INFERRED, most likely)

- Gate read: pm_options.c:~150-160 sets pm_opt.log from options.ini, then `pm_log_set_file(pm_opt.log)`.
- Gate test: vlog, pm_log.c:341 `if (!log_file_on) return;` (before the level test, the queue and the socket).
- Writer sleeps with no timeout while the gate is off: pm_log.c:608-609 `else LightEvent_Wait(&ev_wake);`.
- Result: writer never reaches accept_client (pm_log.c:626). The connection waits in the backlog. Zero lines. log.txt is also not written.
- The writer's first accept happens only after its first 250 ms wait (pm_log.c:608). pm_options_init runs about 20 lines later in pm_main.c:636, so the gate normally closes before the first accept.
- The push line (push_3ds.sh, `livelog "$@"`) does not pass `log=`. So the saved ini value decides the gate.
- pm_debug_value matches only whole words (pm_warp.c:56-69, edge check then `=`). `log=` does not match `loglevel=`.

Open point: LOGCON did not change how the `log` key is read or written (pm_options.c diff: only `conlevel` lines added). An ini with log=0 would have failed before LOGCON too. The memory note project_console-fps-logging-off says Logging was turned off on 2026-10-31 for fps tests. This could explain a log=0 ini. The user reports that pushes before LOGCON returned logs. The dates do not fit unless Logging was turned off after the last working push. The user must confirm the Logging row (see check 1).

### H2. Writer thread failed to start (INFERRED, second)

- start_writer (pm_log.c:643-654) waits 1 s for `writer_started`. If both tries fail, pm_log.c:813-816 logs "writer thread failed" and returns with writer_on = 0.
- pm_log_poll then accepts the client (pm_log.c, pm_log_poll), so "connected" is printed. No writer drains the queue, so no line is sent.
- The failure line itself is queued and never drained, so no bottom-screen message shows the failure.
- log.txt is also empty in this case. The gate check does not tell H1 from H2 by itself.

### H3. Writer blocked before the first send (INFERRED, low)

- out_batch order: fb_put (SD write path, pm_log.c:~440-470), then ring_add, then send. fb_put waits on fsink_lock only when fb is full. A stalled SD write holds fsink_lock.
- A blocked writer sends nothing. The accept runs at the top of the loop, so a block after accept would still send the ring replay only if lines were already drained.
- No N3DS-specific code is on this path (see f below). Low probability.

### H4. con_lock blocks the socket send (REJECTED)

- con_lock is taken only by pm_con_lock/pm_con_unlock/pm_log_console_ready (pm_log.c:326-333) and con_flush (pm_log.c:~535-539).
- out_batch uses fb_lock (cb_put, pm_log.c:~482) and drain_lock. It never takes con_lock.
- 45fca7dd0 added only `con_lines += ...` inside con_lock (pm_log.c, con_flush). No new lock on the socket path.

### H5. Level test drops all lines (REJECTED, CONFIRMED with the push switches)

- vlog pm_log.c:342 `if (lv > pass_lv) return;`. pass_lv = max(log_level, console level) (pm_log.c:60).
- The push sets `loglevel=debug`. pm_log_set_switches (pm_log.c:~214-220) sets log_level = 3. pm_options_init (pm_options.c:~156-160) keeps it. So pass_lv = 3 and every level passes.
- The ini loglevel does not matter when the switch is present.

### H6. Crash or hang before the first line (INFERRED, unlikely)

- Boot order (pm_main.c:611-641): pm_log_set_switches, pm_log_init(live) (listen up), pm_log_start (cpu_apply, aptHook, writer thread, sink thread), pm_log_live_status, pm_wd_start, APT_CheckNew3DS, "Paper Mario 3DS" first line (pm_main.c:~628).
- A hang in pm_log_start would stop pm_log_poll too (it runs in the frame loop), so no accept. This fits 0 lines.
- The user says the N3DS game runs. That points away from a boot hang. Check the picture on the bottom screen to confirm.

### H7. N3DS-specific code near boot (CONFIRMED: no effect on the gate, accept or send)

- pm_new3ds is read in pm_input.c, pm_options.c (dpad) and the HUD text (pm_main.c:577-578). It is not read in pm_log.c.
- APT_CheckNew3DS and osSetSpeedupEnable(true) run at pm_main.c:624-626, after pm_log_start. No effect on the gate or writer.
- The watchdog starts at pm_main.c:~623 (pm_wdog.c:71-77). It exits the process on a hang only.
- No L2 cache code exists in the source (grep for L2C, l2c, "L2 cache" found nothing).

## 45fca7dd0 (LOGCON) diff, checked for the log path

Changes that touch the log path:
1. vlog level compare: `lv > log_level` became `lv > pass_lv` (pm_log.c:342). Level only. Gate line (341) unchanged.
2. drain_locked splits the file/socket and console copies (tf, tc). Socket copy is unchanged for tf lines.
3. out_batch: `n <= 0` path now calls cb_put and returns. Socket code not reached either way.
4. pm_opt initializer (pm_options.c:12). See the shift bug below.
5. Options keys: `conlevel` added. `log` read and write unchanged.

Not changed: writer_main, accept_client, pm_log_poll, start_writer, io_gate, the log gate test at pm_log.c:341.

So LOGCON does not block the socket. A change in the ini content (log=0) or a user toggle after the last working push fits better than a code change.

## Bug found: PmOpt initializer shifted by LOGCON (CONFIRMED by count)

pm_port.h:202-215 declares 21 PmOpt fields. The initializer at pm_options.c:12 holds 20 values after 45fca7dd0. The conlevel value was added and one zero was not removed, so every field after conlevel moves one slot:

| Field | Correct value | Value after LOGCON |
|---|---|---|
| fixedseed, dpad_mode, warp | 0 | 0 (unchanged) |
| warp_entry | 0 | 5 |
| dz_pad | 5 | 0 |
| dz_cs | 0 | 500 |
| rep_delay | 500 | 200 |
| rep_rate | 200 | 1 |
| fskip | 1 | 1000 (true) |
| hud_ms | 1000 | 0 |
| pos_hud | 0 | 0 |

Effects when the key is absent from options.ini (fresh install, or an old ini without the key):
- dz_cs = 500: the N3DS C-Stick dead zone (pm_input.c:193) blocks every C-Stick press.
- rep_rate = 1: menu key repeat every 1 ms.
- hud_ms = 0: bottom memory rows refresh each frame (pm_main.c:518).
- pm_options_write at first boot (pm_options.c:167) saves the wrong values. Later reads skip out-of-range values (pm_options.c:~142, `v > hi`), so the wrong value persists.

Fix (one line, pm_options.c:12): the zeros after conlevel must be 0,0,0,0 then 5:
`PmOpt pm_opt = {0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 5, 0, 500, 200, 1, 1000, 0};` (21 values). Not applied: this row is read-only.

This shift does not explain the log symptom. The log gate is field index 7 and is unchanged.

## Discriminating checks (cheapest first)

1. No push, no Azahar: on the N3DS, open the options panel, Debug tab, and read the row "Logging" (pm_options.c row 6, shows pm_opt.log, the value for this run). Off means H1 is likely. On means H1 is ruled out for this run.
2. One push with the switch `log=on` added. The gate is set by pm_log.c:218 and pm_options.c (`pm_debug_value("log")`). The ini is not rewritten by this switch (sv_log keeps the saved value).
   `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.224 audiostat texstat pacestat logstat phase phsum loglevel=debug conlevel=warn log=on`
   Lines arrive: H1 confirmed. No lines: H1 ruled out, go to H2.
   (The orchestrator runs this push. The line is for the orchestrator to check against rule 6.)
3. If check 2 still gives 0 lines: H2 (writer failed) or H3. Neither has a screen signal today. The next step is a code change: write the writer state (`writer_th`, `writer_on`, the return of start_writer) to a file on the SD card at boot, or to the bottom screen before the console starts. That is a new row; not done here.
4. Check the bottom screen while the N3DS runs: if no game text shows, the console path is quiet too. This does not separate H1 from H2 (both silence the console).

## Open items for the user

- Date of the last working N3DS push, and whether Logging was turned off after it (memory note says 2026-10-31).
- Whether the N3DS options.ini has log=0 (check 1 answers this without FTP).
- Whether the O3DS and N3DS ini files both lack the dz_cs and rep_rate keys (decides how much of the initializer shift matters on each console).
