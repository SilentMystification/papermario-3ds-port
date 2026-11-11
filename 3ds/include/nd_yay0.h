/* YAY0FAST: Yay0 decoder for the map assets and the sprite loads (3ds/src/nd_yay0.c). Pure C. */
#ifndef ND_YAY0_H
#define ND_YAY0_H

#include <stdint.h>

/* Returns 1 on success. Same output and same failure cases as the former pm_map.c yay0_decode_raw. */
int nd_yay0_raw(const uint8_t* src, uint32_t src_len, uint8_t* dst, uint32_t dst_len);

#endif
