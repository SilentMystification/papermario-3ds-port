/* nd_snap_link_stub.c: host link stub for the snap symbols that nd_direct.c (nd_glyph_draw, NDR3) references. The host test of
 * nd_direct_test.c includes nd_direct.c and links no nd_snap.c. The snap is never armed on the host (nd_snap_arm stays 0). */
unsigned char nd_snap_arm;
void nd_snapchk_tri(void) {}
void nd_snap_render(void) {}
