# Row LINRACE: crash in the linear allocator from the prefetch thread (O3DS XL, 2026-11-09)

## User facts
XL crashed (main 21e3b029b push, build 3651) after play in dgb_* / arn_* (last log lines: arn_02 load). Luma dump
build3ds/hw_o3ds_dumps/crash_dump_00000001.dmp: data abort, process 3dsx_app, pc 0040c88c, far e085dfa1.

## Stack (orchestrator, addr2line on 3ds/logs/builds/pm_3ds_3651.elf)
addrMapNodeComparator (libctru allocator/addrmap.h:16) <- do_insert (rbtree_insert.c:19) <- linearMemAlign (linear.cpp:51)
<- sample_build (pm_dspaud.c:116, `linearMemAlign(need, 128)`) <- pm_dspa_predecode (pm_dspaud.c:187) <- pf_main (pm_prefetch.c:54)
<- _thread_begin. The prefetch thread allocates linear memory while other threads (GPU command buffers, textures, audio) allocate
or free linear memory; the libctru linear allocator (MemPool + AddrMap rbtree) has no lock, so the tree gets a wild node.

## Start state
`sh 3ds/tools/wt_new.sh LINRACE`, branch LINRACE. PM_ROW=LINRACE, PM_LANE=LINRACE. One coder agent.

## Steps
1. Code check: confirm libctru 2.7.0 linearMemAlign/linearFree take no lock (the devkitPro source in the build image, or the
   disassembly), and list every linearAlloc/linearMemAlign/linearFree/vramAlloc/vramFree call site in 3ds/src (15+) with its thread.
2. Fix: one port wrapper pair pm_linear_alloc(size, align) / pm_linear_free(p) with a LightLock (or RecursiveLock if a site nests);
   every call site uses it (no direct linearMemAlign/linearFree left; same for vram* if more than one thread uses them). A grep in
   nd_host_test.sh fails the host test on a direct call outside the wrapper.
3. Gates: nd_host PASS (with the grep); build OK; memcheck OK; one Azahar run of a scene with music changes (aud_mac01_lag or
   toad_nap_aud) with no allocation fault line; spot_mac01_det byte-equal. Console (user): play dgb/arn with music changes.
4. Commit; delete this row file in the completing commit. Budget: 3 Azahar starts.

## Touch only
3ds/src/* call sites of linear/vram alloc and free, a new pm_linear.h/.c (or pm_mem.c), nd_host_test.sh.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
