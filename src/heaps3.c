#include "common.h"

/* _heap_create stores its node at the 16-byte-aligned address and
 * _heap_malloc reads the node from the same base. These must already
 * be aligned or the node is written past the header malloc looks at. */
BSS __attribute__((aligned(16))) u8 heap_generalHead[GENERAL_HEAP_SIZE];
BSS __attribute__((aligned(16))) u8 heap_spriteHead[SPRITE_HEAP_SIZE];
BSS u16 gFrameBuf0[FRAME_BUFFER_SIZE / 2];
BSS u16 gFrameBuf1[FRAME_BUFFER_SIZE / 2];
BSS u16 gFrameBuf2[FRAME_BUFFER_SIZE / 2];
