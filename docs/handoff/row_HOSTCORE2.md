# Row HOSTCORE2: the core 0 draw path on the host, one NdDrec per draw at the PICA word boundary (Sonnet, design core)

Model: Sonnet (plan v3, accepted by the user). Reason: the host link closure of the core 0 files and the host side of 65
citro3d and GPUCMD symbols appear only while it links. No exact step list exists before. Why not Opus: the record format
(HOSTCORE1) and the word decoder (FRAMEHASH1) exist. This row joins them.

## Facts (ORACLE8, main b8b314758)
- 7 files call citro3d or GPUCMD (`grep -l "C3D_\|GPUCMD_" 3ds/src/*.c`): nd_mtx.c, nd_tex.c, nd_tmpl.c (already host-built by
  nd_host_test.sh without citro3d), pm_gpu.c, pm_tex.c, pm_main.c, pm_phase.c. The draw path needs pm_gpu.c and pm_tex.c, not
  pm_main.c (frame loop) or pm_phase.c (timers).
  65 distinct C3D_/C3Di_/GPUCMD_ symbols. Two emit paths reach the GPU: citro3d (C3Di_UpdateContext + C3D_DrawElements, the
  old walker path, flush_draw) and the native emit (nd_native_submit pm_gpu.c:1205-1250: nd_emit_draw or blk_put writes the
  words into the command buffer, pm_gpu.c:1230-1231).
- The citro3d source is not in the toolchain (docker pm3ds-build:1 has only /opt/devkitpro/portlibs/3ds lib and headers). So a
  host build cannot run the real citro3d. pm_comb.c, nd_emit.c, nd_tmpl.c, nd_model.c, nd_op.c, nd_mtx.c already build on the
  host (3ds/tools/nd_host_test.sh:30-63).
- 3ds/include/nd_drec.h (HOSTCORE1, merged): NdDrec {seq, kind, n, regs, tex[3], vtx, idx, mtx} and a text writer.
  3ds/src/nd_fhash.c (FRAMEHASH1, merged 2026-11-12): PICA command walk. FRAMEHASH2 adds the per-draw content walk.
- Correction 1 (ORACLE8, main 344822275): core 0 entry points: pm_frame_walk(const PmFrame*) (pm_nusys.c:138, a frame of
  tasks), pm_key_show (pm_nusys.c:228), nd_stream_run (nd_rec.c:586, one recorded stream). nd_walk_frame does not exist.
- Correction 1: pm_gpu.c:1-6 includes <3ds.h>, <3ds/gpu/gpu.h>, <3ds/gpu/registers.h>, <citro3d.h>. pm3ds-host:1 has none,
  3ds/include and 3ds/stub_include have none. Precedent: 3ds/tests/au2/3ds.h is a host shim of the libctru calls of one file.
  pm_gpu.c:13 and :18 include pm_shader5_shbin.h and pm_shader4_shbin.h, which the device build generates (not in the tree).
  pm_gpu.c reads raw C3D context bytes: `__C3D_Context + 32` is the flags word (:636, :1233, :2320), and the gx queue is its
  first member (:2458).
- Golden masters compare the OLD code of a file with its converted code on the same host build (plan section 5). Both paths
  must give the same record for the same picture. The two emit paths write different but equal register sets (redundant writes,
  order, citro3d's extra state). So a record of "registers in emit order" would differ between equal draws.

## Design (fixed. The coder chooses the inner steps)
1. Boundary: the PICA command words. On the host the command buffer is a host array. Every path writes words into it (the
   native emit as today, citro3d through the host side of item 2).
2. citro3d on the host: a test-only C file 3ds/tools/host_c3d.c that implements the C3D_/GPUCMD_/gx calls the 7 files use, with
   the register writes citro3d makes for them (TexEnv stages, blend, depth, alpha test, scissor, viewport, attribute and buffer
   setup, texture units, uniforms, draw triggers). Source of truth for each write: the citro3d disassembly in build3ds/pm_3ds.elf
   (`arm-none-eabi-objdump -d --disassemble=<fn>` in docker pm3ds-build:1, as the oracle read C3D_FrameBegin). Name in the
   commit the functions and the registers each writes. It is a test input, never linked into the 3ds build (no stub there).
3. Record: at each draw trigger (0x22E or 0x22F) one NdDrec. regs = the EFFECTIVE value of a fixed register list at the draw,
   sorted by register, not the writes (the coder fixes the list from the registers the two paths write and puts it in the
   commit. It holds at least: TEV 0x0C0-0x0FD, 0x100-0x107 blend, depth, alpha, 0x080-0x09E texture units, 0x0E0
   and 0x0E1 texenv buffer, 0x041-0x048 viewport and scissor, 0x065-0x068, 0x06E, 0x2B0-0x2BF and 0x2C0 uniform words as the
   current float uniform table of the vertex shader, 0x229, 0x22A, 0x227 bit 31, 0x228, 0x25E). Address registers carry 0, their
   content goes to tex, vtx, idx (FRAMEHASH2's ranges). mtx: none (the uniform table holds the matrices).
4. Driver: 3ds/tools/host_core.c with `host_core_begin(FILE*)`, `host_core_end()`: a host frame around calls into the core 0
   entry points (pm_frame_walk, pm_key_show, nd_stream_run). GMHARN (next row) calls it per game file.
5. nd_host: host.d/hostcore.sh builds it and runs one self-test: a fixed draw through the native emit (an NdDraw fixture) and the
   same draw through the citro3d path (host_c3d with the state the old walker sets for it) give the same NdDrec text.

## Readers and writers
Writers: host_c3d.c (citro3d words), pm_gpu.c and nd_emit.c (native words, unchanged), host_core.c (records). Readers:
nd_drec_write, GMHARN, GMCOV. List every file the host link pulls in and every host-side function in the commit.

## Places
New 3ds/tools/host_c3d.c, 3ds/tools/host_core.c, 3ds/tools/host_core.h, 3ds/tests/host.d/hostcore.sh, and the host header dir
3ds/tests/hostcore/include/ (correction 1): 3ds.h, citro3d.h, 3ds/gpu/gpu.h (types, prototypes and the libctru inline helpers the
files use) and 3ds/gpu/registers.h (a copy of libctru's, zlib license header kept, from pm3ds-build:1
/opt/devkitpro/libctru/include/3ds/gpu/registers.h). Struct layouts that pm_gpu.c reads as bytes (C3D context flags at +32,
gxCmdQueue_s first) follow the real headers of pm3ds-build:1 (/opt/devkitpro/libctru/include, /opt/devkitpro/portlibs/3ds/include)
and the disassembly. The shader headers: the two generated .shbin files go to 3ds/tests/hostcore/ with the hash of their 3ds/shaders
source in a comment, or the test copies them from the build output: the coder chooses and writes why in the commit.
The host build adds `-I 3ds/tests/hostcore/include` before the other include paths. It is never on the 3ds build path. No edit of a 3ds/src file
unless a file cannot compile on the host for a reason outside citro3d: then one `#ifdef PM_HOST` per file is not allowed (no
switches): stop and hand back the file and the reason.

## Gate (expected values)
- nd_host PASS with `hostcore ok` (the two paths of item 5 give byte-equal record text, and a seeded fault (one texel byte of the
  fixture changed for one path) gives a difference only in its `t 0` line, shown before the fix).
- Two runs of the self-test give equal text. 3ds build untouched: `sh 3ds/build.sh` memcheck OK. No Azahar. Budget 0.

## Stop
A core 0 file that needs a source edit to build on the host, a citro3d function whose register writes the disassembly does not
show, or two equal draws that give different records: stop, commit, hand back (2 tries per named cause). Delete this row file in the
completing commit.
