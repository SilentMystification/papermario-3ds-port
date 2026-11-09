# Row SBLK2: super block upgrade, opaque shimmer and slow garbled orbs (user, 2026-11-09; replaces row FX1)

## User facts
User (O3DS, de415917f): the animation between choosing the partner and the flash is not properly transparent. The 3 circles
("dots") are garbled and very laggy on the console; the triangles around them distort and glitch. User: FX1 got stuck on one path for
a long time; this row must not. Scenario: sbk56_superblock (and sbk56_orbdbg_det), until "superblock: effects done".

## Effects (oracle, code read, main 00a4e1901; src/world/common/entity/SuperBlock.inc.c)
Confirm -> SwitchToPartner (:539) -> Wait 10 -> ShowUpgradeEffects (:451-485): GatherEnergyFX (:457 -> :237) = fx_radial_shimmer(4)
75 frames; WhiteScreenFlash (:461-464); RadiateFaintEnergyFX radial_shimmer(9) + sparkles (:472-477); AnimateEnergyOrbs (:480 ->
:263-380) = 3 x fx_motion_blur_flame (the 3 circles with triangles). (A) = radial_shimmer 4. (B) = motion_blur_flame.
Proven by FX1 (do not re-test): block body texture right; energy_orb_wave glow matches the N64 formula at f524; orbdbg showed the
shimmer draws used TEX0*SHADE with an RGBA16 texture, which no shimmer list uses.

## Start state
`sh 3ds/tools/wt_new.sh SBLK2`, branch SBLK2. PM_ROW=SBLK2, PM_LANE=SBLK2. One Haiku agent.

## Suspects, in order. One check run each; a false check moves to the NEXT suspect, never a variant.
A1 Premise: the shimmer triangles draw with leftover state: the segment-9 setup list (dlist1 = D_E0066C8C[4], radial_shimmer.c:333,
   gSPSegment 9 at :314) is not applied or resolves through a stale segment-9 base (recorder inlines static lists on core 1 since
   D11a, nd_rec.c rec_inline). Check: one log line per draw in dlist2 during the effect: seg9 base vs effect->shared->graphics, td[0]
   fmt/siz, comb_w0/w1, cycle type, r_blend. N64: I4, the D_090039A8 combine, 2-cycle, blend 1. Values equal N64 -> A1 false, go A2.
   Fix if true: where the state is lost (rec_inline segment resolution, or segment-9 lists replayed as G_DL); not in the TEV.
A2 Premise: state right, but unit 1 (I4 at tmem 0x100) reads alpha 1 (pm_tex_i8(1) only for off-screen aux slots). Check: same line
   plus i8_1 and the final TEV alpha source. i8_1 = 1 or alpha from R -> false, go A3. Fix: unit-1 intensity alpha for CPU I4/I8 slots
   (pm_tex.c, nd_comb.c dual_bind).
A3 Premise: draw order/depth (flag 20 / CLOUD_NO_ZCMP in the wrong pass). Check: render-task order line of the effect vs glow and
   block. Same as N64 -> hand back with all printed values.
B1 Premise: motion_blur_flame (src/effects/motion_blur_flame.c:156-273) loads the current colour buffer per strip (:248); each texrect
   sets grab_dirty, so the next strip copies again (pm_gpu.c grab_impl :2188-2245: flush + C3D_FrameSplit + full-screen copy), up to
   about 48 splits per frame: the lag; mid-frame copies out of order: the garbling. Check: prof cnt spl per frame (sc_spl) and census
   "sp<n> gr<n>" (nd_cen.c:510) with the orbs vs before. spl <= 3 -> false, go B2. Fix: one copy per effect per frame:
   pm_gpu_grab_hold(1)/(0) around motion_blur_flame_appendGfx (TARGET_3DS); while held grab_impl reuses the frame copy.
   Files: motion_blur_flame.c, pm_gpu.c grab_impl, nd_target.c.
B2 Premise: flame texture re-uploaded every frame. Check: prof texture uploads/KB during the orbs vs before. No rise -> go B3.
B3 Premise: console-only copy fault (wide x1 400 rows, slot origin). Check: console offdump=<frame> (NEEDS CONSOLE). Slot right -> hand back.

## Touch only
The files named in the suspect being fixed. nd_rec.c and nd_comb.c are also edited by row HC4: keep edits there minimal and say so.

## Done criteria
nd_host PASS, memcheck OK. Shot during GatherEnergyFX: shimmer transparent (world visible through it). Shot of the 3 orbs: distortion,
no garbage. spl about 1-3 with the orbs (Azahar census; console check by the user). Byte gate: spot_mac01_det vs stored base.
Delete this row file and docs/handoff/row_FX1.md in the completing commit. Budget: 8 Azahar starts. At most 2 tries per named cause.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops. If every suspect is false, hand back with the values.
