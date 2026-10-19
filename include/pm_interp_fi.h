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
#endif
