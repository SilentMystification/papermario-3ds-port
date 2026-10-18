/* F03: encode a viewport depth D (0 near .. 32704 far) into the N64 z-buffer word that is_point_visible decodes. */
#ifndef PM_DEPTH_H
#define PM_DEPTH_H
static inline unsigned short pm_depth_encode(int d) {
    static const unsigned bias[8] = { 0x00000, 0x20000, 0x30000, 0x38000, 0x3C000, 0x3E000, 0x3F000, 0x3F800 };
    static const unsigned shift[8] = { 6, 5, 4, 3, 2, 1, 0, 0 };
    unsigned v, e;
    if (d < 0) d = 0;
    if (d > 32704) d = 32704;
    v = (unsigned)d << 3;
    e = v < 0x20000 ? 0 : v < 0x30000 ? 1 : v < 0x38000 ? 2 : v < 0x3C000 ? 3 : v < 0x3E000 ? 4 : v < 0x3F000 ? 5 : v < 0x3F800 ? 6 : 7;
    return (unsigned short)((e << 13) | ((((v - bias[e]) >> shift[e]) & 0x7FFu) << 2));
}
#endif
