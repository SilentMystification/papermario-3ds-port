/* nd_fs.h: the field-set ids of the typed unit (core 1 names). Same values and order as the unit ids in nd_op.h (ND_FS_*), so
 * the setters and the field readers can name a field by its set id. A field set names the state a unit writes, not the gbi op.
 * The ids are 0x20 + k (k < 32). Host-built (nd_fs_test.c). */
#ifndef ND_FS_H
#define ND_FS_H
#include <stdint.h>

enum {
    ND_FS_BASE = 0x20,
    ND_FS_OMH = 0x20, /* the other mode high word: a = shift | length << 8, b = value */
    ND_FS_OML,        /* the other mode low word: as the high word */
    ND_FS_GEOM,       /* the geometry mode: a = the clear mask (w0 low 24 bits), b = the set bits */
    ND_FS_PRIM,       /* the primitive colour: a = the LOD word (16 bits), b = colour */
    ND_FS_ENV,        /* the environment colour: b = colour */
    ND_FS_FOG,        /* the fog colour: b = colour */
    ND_FS_BLEND,      /* the blend colour: b = colour */
    ND_FS_FILL,       /* the fill colour: b = colour */
    ND_FS_PRIMZ,      /* the primitive depth: b = z << 16 | dz */
    ND_FS_SCISSOR,    /* the scissor: a = ulx << 12 | uly, b = lrx << 12 | lry (10.2 fields; the wide sign rule runs on core 0) */
    ND_FS_TEXTURE,    /* the texture enable: a = on (0 or 1), b = ss << 16 | ts */
    ND_FS_MW,         /* the move word: a = kind << 16 | arg (move kinds below), b = value */
    ND_FS_RARE,       /* the convert, key and key colour words: a = the rare kind, b = w1 */
    ND_FS_COMB,       /* the combine: a = w0 low 24 bits, b = w1 (the walker keeps the full word: the combine word, see below) */
    ND_FS_END,        /* the end of a typed table (render mode table); never in a stream run */
    ND_FS_DROP,       /* the debug mark of a dropped op: the next op is one the recorder shadow dropped; core 0 checks it */
    ND_FS_TIMG,       /* the texture image: a = fmt << 21 | siz << 19 | (width - 1), b = image address (a segment address stays a value) */
    ND_FS_TILE,       /* the tile: a = fmt << 21 | siz << 19 | line << 9 | tmem, b = tile << 24 | palette << 20 | cmt, maskt, shiftt, cms, masks, shifts */
    ND_FS_TILESIZE,   /* the tile size: a = uls << 12 | ult, b = tile << 24 | lrs << 12 | lrt (10.2 fields) */
    ND_FS_LOADBLOCK,  /* the block load: a = uls << 12 | ult, b = tile << 24 | texels - 1 << 12 | dxt */
    ND_FS_LOADTILE,   /* the tile load: as the tile size */
    ND_FS_LOADTLUT,   /* the palette load: a = 0, b = tile << 24 | (count - 1) << 14 */
    ND_FS_VTX,        /* the vertex load (geometry ops): a = n << 12 | (v0 + n) << 1, b = vertex address */
    ND_FS_TRI1,       /* the triangle: a = v0 * 2 << 16 | v1 * 2 << 8 | v2 * 2 */
    ND_FS_TRI2,       /* the two triangles: a = the first triangle, b = the second (as the triangle a) */
    ND_FS_MTX,        /* the matrix: a = the w0 low 24 bits (flags in the low byte), b = matrix address */
    ND_FS_POPMTX,     /* the matrix pop: b = bytes (64 per matrix) */
    ND_FS_MOVEMEM,    /* the memory move: a = the w0 low 24 bits (offset << 8 | index), b = address */
    ND_FS_CALL,       /* the list call (control ops): a = 1 for the branch form, b = list address */
    ND_FS_RECT,       /* the rectangle: a = lrx << 12 | lry, b = texture flag | tile << 24 | ulx << 12 | uly (10.2 fields).
                       * A texture rectangle keeps its two half units after it (their b: s, t and dsdx, dtdy) */
    ND_FS_MISC,       /* a = kind << 16 | arg: the list start of an in-line state list, the native-only payload
                       * (arg = kind << 8 | on), the vertex modify (arg = where << 8 | vertex, vertex 255 when above),
                       * the colour image (arg = siz << 12 | width - 1, b = image address), no work */
    ND_FS_END_
};
enum { ND_FS_MW_NONE, ND_FS_MW_NUMLIGHT, ND_FS_MW_LIGHTCOL /* arg = light k */, ND_FS_MW_FOG, ND_FS_MW_SEG /* arg = segment */ };
enum { ND_FS_RARE_CONV, ND_FS_RARE_KEYR, ND_FS_RARE_KEYGB };
enum { ND_FS_MISC_NOP, ND_FS_MISC_LST, ND_FS_MISC_ND, ND_FS_MISC_MODVTX, ND_FS_MISC_CIMG, ND_FS_MISC_EP }; /* NAT4A: a core 1 cut mark, no state */
#define ND_FS_RECT_TEX 0x80000000u /* texture flag of the rectangle b: a texture rectangle (else a fill) */

#define ND_FS_W0(id, a) (((uint32_t)(id) << 24) | ((uint32_t)(a) & 0xffffffu))
#define ND_FS_COMB_W0(a) ((uint32_t)0xfcu << 24 | ((uint32_t)(a) & 0xffffffu)) /* the combine word of a typed combine (model keys) */
#define ND_FS_A_RENDERMODE (3u | 29u << 8) /* the a field of the render mode set (shift 3, 29 bits) */

#endif
