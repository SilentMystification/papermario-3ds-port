#ifndef ND_SCI_H
#define ND_SCI_H
/* WCLIP1: one scissor rule for the x edges (plan_widewin.md 4). Pure C, host-testable. */
int nd_sci_full(int x0, int y0, int x1, int y1);
void nd_sci_x(int x0, int y0, int x1, int y1, int mode, int ui, int* top, int* bottom);
#endif
