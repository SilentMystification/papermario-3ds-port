#ifndef LD_ADDRS_H
#define LD_ADDRS_H

/* Code overlays are already linked into the 3dsx. A zero-length copy leaves them alone.
 * Asset ranges are ROM file offsets. dma_copy treats the pointer value as that offset. */
#define engine4_ROM_START ((u8*)1)
#define engine4_ROM_END   ((u8*)1)
#define engine4_VRAM      ((u8*)1)
#define engine1_ROM_START ((u8*)1)
#define engine1_ROM_END   ((u8*)1)
#define engine1_VRAM      ((u8*)1)
#define evt_ROM_START     ((u8*)1)
#define evt_ROM_END       ((u8*)1)
#define evt_VRAM          ((u8*)1)
#define entity_ROM_START  ((u8*)1)
#define entity_ROM_END    ((u8*)1)
#define entity_VRAM       ((u8*)1)
#define engine2_ROM_START ((u8*)1)
#define engine2_ROM_END   ((u8*)1)
#define engine2_VRAM      ((u8*)1)
#define font_width_ROM_START ((u8*)1)
#define font_width_ROM_END   ((u8*)1)
#define font_width_VRAM      ((u8*)1)

/* ver/us/splat.yaml logos segment: 0x1FE1B0 .. 0x2191B0 */
#define logos_ROM_START ((u8*)0x1FE1B0)
#define logos_ROM_END   ((u8*)0x2191B0)

/* Storybook pages live in the asset table. A placeholder keeps the intro script linked. */
/* ver/us/splat.yaml: title/bg_1 through title/bowser_silhouette. */
#define title_bg_1_ROM_START ((u8*)0x2191B0)
#define title_bg_1_ROM_END   ((u8*)0x24B7F0)

#endif
