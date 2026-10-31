# NL1-inv: next push without the Y press

Question: when the game exits, can it make the Homebrew Launcher (hbmenu) accept the next 3dslink push without a person pressing Y?

Short answer: no, not through libctru and not through hbmenu. Two other paths can remove the Y press. Both use the Luma3DS service `hb:ldr`. Neither path was tested.

Sources read: libctru headers and `libctru.a` in image `pm3ds-build:1` (read only), `3ds/push_3ds.sh`, `3ds/src/pm_main.c`, `3ds/src/pm_mem.c`, `docs/handoff/plan_console_harness.md`, and `docs/handoff/HANDOFF_2026-10-23.md`. The image and this PC do not contain the hbmenu or Luma3DS source. Each claim about them is INFERRED.

## 1. Does libctru set the next program to load?

- There is no `envSetNextLoad` in libctru. CONFIRMED. A grep of `/opt/devkitpro/libctru/include` finds no `NextLoad` or `hbldr`. `envSetNextLoad` and nxlink are libnx (Switch) items.
- `3ds/env.h` lines 1-75 has only getters: `envIsHomebrew` (18), `envGetHandle` (28), `envGetAptAppId` (34), the heap sizes (43, 52), `envGetSystemArgList` (61), and `envGetSystemRunFlags` (70). CONFIRMED.
- `3ds/services/apt.h` 239-263 has the chainloader: `aptClearChainloader`, `aptSetChainloader(u64 programID, u8 mediatype)`, `aptSetChainloaderToCaller`, `aptSetChainloaderToSelf`, and `aptSetChainloaderArgs(deliverArg, size <= 0x300, hmac)`. CONFIRMED. These functions launch a title ID. They do not take a 3dsx path or an argv list.
- In `libctru.a` `apt.o`, `aptExit` reads `aptFlags` bit 30 (offset 0x58). It sends GetAppletManInfo (0x00050040) and IsRegistered (0x00090040). Then it sends PrepareToDoApplicationJump (0x00310100) and DoApplicationJump (0x00320084) with `aptChainloadTid`, `aptChainloadMediatype` and the deliver arg (offsets 0x14c-0x328). CONFIRMED by disassembly. So the jump runs inside the normal `return 0` from `main` (pm_main.c 690). No other exit call is necessary.
- The argv of a 3dsx comes from `__system_arglist`: a u32 count followed by NUL-separated strings. `__system_initArgv` (initArgv.o) removes a last argument of 17 characters that ends in `_3DSLINK_` (rodata string `_3DSLINK_`, offsets 0xf4-0x14c). It reads the first 8 hex characters into `__3dslink_host`. CONFIRMED. `pm_mem.c` 34-46 and `pm_main.c` 674 test the same two markers: argv[0] starts with `3dslink:`, or the last argument is `XXXXXXXX_3DSLINK_`.
- On Luma3DS, the call that sets "the next 3dsx with this argv" is the Luma loader service `hb:ldr`, not libctru. Command 2 is SetTarget (path, static buffer 0). Command 3 is SetArgv (the same count-plus-strings block, static buffer 1). hbmenu calls it in `source/loaders/rosalina.c`. INFERRED from the hbmenu and Luma3DS sources, which this investigation could not open.

## 2. Does the netloader accept a push after the game exits?

- No. After the 3dsx exits, hbmenu starts again with its netloader off. The netloader starts only when a person presses Y. INFERRED. Sources: plan_console_harness.md 54 ("After quit, the launcher is open, but its netloader is off"), HANDOFF_2026-10-23.md 51 ("The console needs Homebrew Launcher netloader running (press Y)"), and hbmenu behavior from memory.
- The host side has no wake message. `push_3ds.sh` 139/144 runs `3dslink -a <ip>`, which only connects to port 17491 (`LINK3DS_COMM_PORT`, `3ds/3dslink.h` 15). A console that does not listen gives "no answer" (push_3ds.sh 165-167). CONFIRMED for the script.
- From memory, hbmenu has no argv switch or file that starts the netloader at boot. INFERRED.

## 3. What can the game do at exit?

(a) Make hbmenu open its netloader: nothing. No call and no argv string does this. INFERRED, because the hbmenu source is not available.

(b) Start itself again with new switches and the same build, with no push. INFERRED.
1. Open `hb:ldr` (`srvGetServiceHandle(&h, "hb:ldr")`).
2. Send SetTarget with the SD path of the 3dsx, without the `sdmc:` prefix, for example `/3ds/pm_3ds.3dsx`.
3. Send SetArgv with `{count, "pm_3ds.3dsx", <switches...>, "XXXXXXXX_3DSLINK_"}`. The last string carries the PC IP from `__3dslink_host`. Then on the next start `netloaded` is true (pm_main.c 674), and the live log server and its switches work as for a push.
4. `return 0` from `main`. Luma loads the target when the launcher title starts again.

Open item: the 3dsx must be on the SD card. A netloaded 3dsx has argv[0] = `3dslink:/...` (pm_mem.c 41), which is not an SD path. Whether hbmenu also writes the received file to `sdmc:/3ds/` is not known (INFERRED: no). A person can put one copy at `sdmc:/3ds/pm_3ds.3dsx` one time. This path serves H2 (a batch of scenarios on one build). It does not serve a new build.

(c) Accept a new build without Y: the game becomes the netloader. INFERRED. On the `next`/`quit` command, the game would do these steps:
1. Answer the UDP 17491 `3dsboot` discovery (or let the host use `-a <ip>`).
2. Accept TCP 17491.
3. Read the name, the length and the zlib stream. `libz.a` is in `/opt/devkitpro/portlibs/3ds/lib`, CONFIRMED.
4. Write `sdmc:/3ds/pm_3ds.3dsx`.
5. Read the argv block.
6. Do (b).

This change is about 150 lines, more than the 40 lines in the NL1 row. The 3dslink wire format is from memory (INFERRED). The protocol needs a check against the 3dslink source before the row starts.

Risk for (b) and (c): Luma may refuse `hb:ldr` to a 3dsx process. INFERRED: Luma gives a 3dsx full service access, so the call is likely to work. The probe in section 4 answers this.

## 4. The one console test

Make a diagnostic build. Do not commit it. The `quit` path (QUIT1) does these steps before `return 0`:
1. Log `stat("sdmc:/3ds/pm_3ds.3dsx")`. This shows whether hbmenu stored the netloaded file on the SD card.
2. Log argv[0] in full.
3. Open `hb:ldr` and log its rc.
4. Send SetTarget `/3ds/pm_3ds.3dsx` and log its rc.
5. Send SetArgv `{3, "pm_3ds.3dsx", "nl1probe", "<host hex>_3DSLINK_"}` and log its rc.

Before the test, a person copies the same build to `sdmc:/3ds/pm_3ds.3dsx` (only if step 1 is false), presses Y one time, and the harness pushes. Then the harness sends `quit`.

Pass: the console starts the game again with no key press. The live log reconnects and shows a `switches` line with `nl1probe`. This confirms path (b).

Fail cases:
- An `hb:ldr` rc that is not 0: path (b) and path (c) are closed. One Y press per scenario stays.
- The console returns to hbmenu: the target was not used. One Y press per scenario stays.
- Step 1 logs exists=1: the netloader stores the file, and (b) needs no hand copy.

Starts: 1 console run.
