

/* Fonts load into the first three. The rest keep the box and bubble art linked. It is blank until the real images come from the ROM. */
unsigned char MsgCharImgNormal[0x6000] __attribute__((aligned(16)));
unsigned char MsgCharImgTitle[0x1000] __attribute__((aligned(16)));
unsigned char MsgCharImgSubtitle[0x1000] __attribute__((aligned(16)));
unsigned char D_802F4560[0x600] __attribute__((aligned(16)));
unsigned char gItemIconRasterOffsets[0x1000] __attribute__((aligned(16)));
unsigned char gItemIconPaletteOffsets[0x1000] __attribute__((aligned(16)));
unsigned char gMessageBoxFrameParts[0x1000] __attribute__((aligned(16)));
unsigned char ui_msg_palettes[0x1000] __attribute__((aligned(16)));
unsigned char ui_msg_arrow_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_background_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_bubble_left_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_bubble_mid_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_bubble_right_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_lamppost_corner_bottomright_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_rewind_arrow_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_corner_bottomleft_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_corner_bottomright_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_corner_topleft_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_corner_topright_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_fill_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_side_bottom_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_side_left_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_side_right_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_sign_side_top_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_star_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_star_silhouette_png[0x800] __attribute__((aligned(16)));
unsigned char ui_msg_lamppost_pal[0x40] __attribute__((aligned(16)));
unsigned char ui_msg_sign_pal[0x40] __attribute__((aligned(16)));
unsigned char ui_msg_rewind_arrow_pal[0x40] __attribute__((aligned(16)));
unsigned char ui_point_right_pal[0x40] __attribute__((aligned(16)));
unsigned char ui_point_right_png[0x800] __attribute__((aligned(16)));

void draw_box() {}
void draw_ci_image_with_clipping() {}
void draw_image_with_clipping() {}
int sfx_play_sound_with_params() { return 0; }
