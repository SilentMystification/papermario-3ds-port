#ifndef PM_INTERP_FI_H
#define PM_INTERP_FI_H
/* F06-3: frame interpolation node calls (PaperBoat FrameInterpolation_RecordOpenChild / CloseChild). Record only, no game state. */
extern void pm_interp_open(unsigned a, unsigned b);
extern void pm_interp_close(void);
#define FI_OPEN(n, t) pm_interp_open((unsigned)(n), (unsigned)(t))
#define FI_CLOSE() pm_interp_close()
/* PaperBoat TAG_ENTRY: object index and id packed in the key */
#define FI_ENTRY(id, idx) (((((unsigned)(idx)) << 16) & 0x0FFF0000u) | ((((unsigned)(id)) << 8) & 0x0000FF00u))
#define FI_TAG(kind, id, idx) ((unsigned)(kind) | FI_ENTRY(id, idx))
/* F06-4: texrect to quad anchor (PaperBoat ScreenAnchorPatches). Only with fps60. pm_anchor_rect returns 1 if it drew the quad. */
extern void pm_anchor_begin(const void* key, unsigned idx, int x, int y);
extern void pm_anchor_end(void);
extern int pm_anchor_rect(int ulx, int uly, int lrx, int lry, int tile, int s, int t, int dsdx, int dtdy);
#endif
