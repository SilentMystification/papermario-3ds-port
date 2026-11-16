#include "common.h"
#include "nd_gtab.h"
#include "filemenu.h"
#include "ld_addrs.h"

#include "charset/charset.h"
#include "nd_st.h"
#include "nd_tx.h"

s32 msg_get_print_char_width(s32 character, s32 charset, s32 variation, f32 msgScale, s32 overrideCharWidth, u8 flags);

extern MessageCharset* MsgCharsets[5];
extern PAL_BIN D_802F4560[80][8];
extern u8 filemenu_glyphBuffer[20][0x80];

u8 filemenu_msg_C6[] = { 0x33, 0x30, 0x21, 0x23, 0x25, 0xFD };   // SPACE[End]
u8 filemenu_msg_C7[] = { 0xFD };                                 // [End]
u8 filemenu_msg_C8[] = { 0xFD };                                 // [End]
u8 filemenu_msg_C9[] = { 0x22, 0x21, 0x23, 0x2B, 0xFD };         // BACK[End]
u8 filemenu_msg_CA[] = { 0x25, 0x2E, 0x24, 0xFD };               // END[End]

s32 filemenu_specialSymbols[] = { (s32)filemenu_msg_C6, (s32)filemenu_msg_C7, (s32)filemenu_msg_C8, (s32)filemenu_msg_C9, (s32)filemenu_msg_CA };

NDN_DECL(filemenu_dl_draw_char_init);

Gfx* filemenu_savedGfxPos = nullptr;
static const u32* filemenu_savedStreamPos; /* DIRECT D6: the frame stream position after the last char (a recorded char does not move gMainGfxPos) */
s32 filemenu_charset = 0;
s32 filemenu_charset_raster_id = 0;
s32 filemenu_char_color = -1;



u8 filemenu_msg_01[] = { 0x33, 0x45, 0x4C, 0x45, 0x43, 0x54, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0xF7, 0x54, 0x4F, 0xF7, 0x53,
                    0x54, 0x41, 0x52, 0x54, 0x1A, 0xFD };
u8 filemenu_msg_02[] = { 0x33, 0x45, 0x4C, 0x45, 0x43, 0x54, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0xF7, 0x54, 0x4F, 0xF7, 0x44,
                    0x45, 0x4C, 0x45, 0x54, 0x45, 0x1A, 0xFD };
u8 filemenu_msg_03[] = { 0x33, 0x45, 0x4C, 0x45, 0x43, 0x54, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0xF7, 0x54, 0x4F, 0xF7, 0x53,
                    0x41, 0x56, 0x45, 0xFD };
u8 filemenu_msg_04[] = { 0x23, 0x4F, 0x50, 0x59, 0xF7, 0x57, 0x48, 0x49, 0x43, 0x48, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0x1F,
                    0xFD };
u8 filemenu_msg_05[] = { 0x23, 0x4F, 0x50, 0x59, 0xF7, 0x54, 0x4F, 0xF7, 0x57, 0x48, 0x49, 0x43, 0x48, 0xF7, 0x46, 0x49,
                    0x4C, 0x45, 0x1F, 0xFD };
u8 filemenu_msg_06[] = { 0x2E, 0x25, 0x37, 0xFD };
u8 filemenu_msg_07[] = { 0x2C, 0x45, 0x56, 0x45, 0x4C, 0xFD };
u8 filemenu_msg_08[] = { 0x30, 0x4C, 0x41, 0x59, 0xF7, 0x34, 0x49, 0x4D, 0x45, 0xFD };
u8 filemenu_msg_09[] = { 0x24, 0x45, 0x4C, 0x45, 0x54, 0x45, 0xF7, 0x26, 0x49, 0x4C, 0x45, 0xFD };
u8 filemenu_msg_10[] = { 0x23, 0x41, 0x4E, 0x43, 0x45, 0x4C, 0xFD };
u8 filemenu_msg_11[] = { 0x23, 0x4F, 0x50, 0x59, 0xF7, 0x26, 0x49, 0x4C, 0x45, 0xFD };
u8 filemenu_msg_12[] = { 0x26, 0x49, 0x52, 0x53, 0x54, 0xF7, 0x30, 0x4C, 0x41, 0x59, 0xFD };
u8 filemenu_msg_13[] = { 0x0E, 0xFD };
u8 filemenu_msg_14[] = { 0x39, 0x45, 0x53, 0xFD };
u8 filemenu_msg_15[] = { 0x2E, 0x4F, 0xFD };
u8 filemenu_msg_16[] = { 0x24, 0x45, 0x4C, 0x45, 0x54, 0x45, 0xFD };
u8 filemenu_msg_17[] = { 0x2F, 0x56, 0x45, 0x52, 0x52, 0x49, 0x44, 0x45, 0xF7, 0x54, 0x4F, 0xF7, 0x2E, 0x45, 0x57, 0xF7,
                    0x24, 0x41, 0x54, 0x41, 0xFD };
u8 filemenu_msg_18[] = { 0x33, 0x41, 0x56, 0x45, 0xF7, 0x2F, 0x2B, 0x1F, 0xFD };
u8 filemenu_msg_19[] = { 0x26, 0x49, 0x4C, 0x45, 0xF7, 0x4E, 0x41, 0x4D, 0x45, 0xF7, 0x49, 0x53, 0xF7, 0x1A, 0xFD };
u8 filemenu_msg_20[] = { 0x0E, 0xFD };
u8 filemenu_msg_21[] = { 0x2F, 0x2B, 0x1F, 0xFD };
u8 filemenu_msg_22[] = { 0x26, 0x49, 0x4C, 0x45, 0xFD };
u8 filemenu_msg_23[] = { 0x57, 0x49, 0x4C, 0x4C, 0xF7, 0x42, 0x45, 0xF7, 0x44, 0x45, 0x4C, 0x45, 0x54, 0x45, 0x44, 0x0E,
                    0xFD };
u8 filemenu_msg_24[] = { 0x2F, 0x2B, 0xF7, 0x54, 0x4F, 0xF7, 0x43, 0x4F, 0x50, 0x59, 0xF7, 0x54, 0x4F, 0xF7, 0x54, 0x48,
                    0x49, 0x53, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0x1F, 0xFD };
u8 filemenu_msg_25[] = { 0x33, 0x54, 0x41, 0x52, 0x54, 0xF7, 0x47, 0x41, 0x4D, 0x45, 0xF7, 0x57, 0x49, 0x54, 0x48, 0xFD };
u8 filemenu_msg_26[] = { 0x26, 0x49, 0x4C, 0x45, 0xFD };
u8 filemenu_msg_27[] = { 0x48, 0x41, 0x53, 0xF7, 0x42, 0x45, 0x45, 0x4E, 0xF7, 0x44, 0x45, 0x4C, 0x45, 0x54, 0x45, 0x44,
                    0x0E, 0xFD };
u8 filemenu_msg_28[] = { 0xFD };
u8 filemenu_msg_29[] = { 0x23, 0x4F, 0x50, 0x59, 0xF7, 0x46, 0x52, 0x4F, 0x4D, 0xFD };
u8 filemenu_msg_30[] = { 0x54, 0x4F, 0xFD };
u8 filemenu_msg_31[] = { 0x48, 0x41, 0x53, 0xF7, 0x42, 0x45, 0x45, 0x4E, 0xF7, 0x43, 0x52, 0x45, 0x41, 0x54, 0x45, 0x44,
                    0x0E, 0xFD };
u8 filemenu_msg_32[] = { 0x25, 0x4E, 0x54, 0x45, 0x52, 0xF7, 0x41, 0xF7, 0x46, 0x49, 0x4C, 0x45, 0xF7, 0x4E, 0x41, 0x4D,
                    0x45, 0x01, 0xFD };
u8 filemenu_createfile_gridData[] = { 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
                                  0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
                                  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,
                                  0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
                                  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x10, 0x0E, 0x0D, 0xF7,
                                  0x00, 0x90, 0x91, 0x20, 0xC6, 0xF7, 0xF7, 0xC9, 0xF7, 0xF7, 0xCA, 0xF7, 0xF7
};
u8 filemenu_msg_33[] = { 0x1F, 0xFD };
u8 filemenu_msg_34[] = { 0x0E, 0xFD };
u8 filemenu_msg_options[] = { 0x2F, 0x50, 0x54, 0x49, 0x4F, 0x4E, 0x53, 0xFD }; // Options[End]
u8 filemenu_msg_opt_fps60[] = { 0x16, 0x10, 0xF7, 0x26, 0x30, 0x33, 0xFD }; // 60 FPS[End]
u8 filemenu_msg_opt_wide[] = { 0x37, 0x49, 0x44, 0x45, 0x53, 0x43, 0x52, 0x45, 0x45, 0x4E, 0xFD }; // Widescreen[End]
u8 filemenu_msg_opt_hud[] = { 0x28, 0x35, 0x24, 0xFD }; // HUD[End]
u8 filemenu_msg_opt_stats[] = { 0x33, 0x54, 0x41, 0x54, 0x53, 0xFD }; // Stats[End]
u8 filemenu_msg_opt_nofog[] = { 0x2E, 0x4F, 0xF7, 0x46, 0x4F, 0x47, 0xFD }; // No fog[End]
u8 filemenu_msg_opt_nolight[] = { 0x2E, 0x4F, 0xF7, 0x4C, 0x49, 0x47, 0x48, 0x54, 0xFD }; // No light[End]
u8 filemenu_msg_opt_back[] = { 0x22, 0x41, 0x43, 0x4B, 0xFD }; // Back[End]
u8 filemenu_msg_opt_on[] = { 0x2F, 0x4E, 0xFD }; // On[End]
u8 filemenu_msg_opt_off[] = { 0x2F, 0x46, 0x46, 0xFD }; // Off[End]
u8 filemenu_msg_opt_log[] = { 0x2C, 0x4F, 0x47, 0x47, 0x49, 0x4E, 0x47, 0xFD }; // Logging[End]
u8 filemenu_msg_opt_loglevel[] = { 0x2C, 0x4F, 0x47, 0xF7, 0x4C, 0x45, 0x56, 0x45, 0x4C, 0xFD }; // Log level[End]
u8 filemenu_msg_opt_fixedseed[] = { 0x26, 0x49, 0x58, 0x45, 0x44, 0xF7, 0x53, 0x45, 0x45, 0x44, 0xFD }; // Fixed seed[End]
u8 filemenu_msg_opt_lv_error[] = { 0x25, 0x52, 0x52, 0x4F, 0x52, 0xFD }; // Error[End]
u8 filemenu_msg_opt_lv_warn[] = { 0x37, 0x41, 0x52, 0x4E, 0xFD }; // Warn[End]
u8 filemenu_msg_opt_lv_info[] = { 0x29, 0x4E, 0x46, 0x4F, 0xFD }; // Info[End]
u8 filemenu_msg_opt_lv_debug[] = { 0x24, 0x45, 0x42, 0x55, 0x47, 0xFD }; // Debug[End]

u8 *gFileMenuMessages[] = {
    [FILE_MESSAGE_NONE]                        nullptr,
    [FILE_MESSAGE_SELECT_FILE_TO_START]        filemenu_msg_01,
    [FILE_MESSAGE_SELECT_FILE_TO_DELETE]       filemenu_msg_02,
    [FILE_MESSAGE_SELECT_FILE_TO_SAVE]         filemenu_msg_03,
    [FILE_MESSAGE_COPY_WHICH_FILE]             filemenu_msg_04,
    [FILE_MESSAGE_COPY_TO_WHICH_FILE]          filemenu_msg_05,
    [FILE_MESSAGE_NEW]                         filemenu_msg_06,
    [FILE_MESSAGE_LEVEL]                       filemenu_msg_07,
    [FILE_MESSAGE_PLAY_TIME]                   filemenu_msg_08,
    [FILE_MESSAGE_DELETE_FILE]                 filemenu_msg_09,
    [FILE_MESSAGE_CANCEL]                      filemenu_msg_10,
    [FILE_MESSAGE_COPY_FILE]                   filemenu_msg_11,
    [FILE_MESSAGE_FIRST_PLAY]                  filemenu_msg_12,
    [FILE_MESSAGE_PERIOD_13]                   filemenu_msg_13,
    [FILE_MESSAGE_YES]                         filemenu_msg_14,
    [FILE_MESSAGE_NO]                          filemenu_msg_15,
    [FILE_MESSAGE_DELETE]                      filemenu_msg_16,
    [FILE_MESSAGE_OVERRIDE_TO_NEW_DATA]        filemenu_msg_17,
    [FILE_MESSAGE_SAVE_OK]                     filemenu_msg_18,
    [FILE_MESSAGE_FILE_NAME_IS]                filemenu_msg_19,
    [FILE_MESSAGE_PERIOD_20]                   filemenu_msg_20,
    [FILE_MESSAGE_OK]                          filemenu_msg_21,
    [FILE_MESSAGE_FILE_22]                     filemenu_msg_22,
    [FILE_MESSAGE_WILL_BE_DELETED]             filemenu_msg_23,
    [FILE_MESSAGE_OK_TO_COPY_TO_THIS_FILE]     filemenu_msg_24,
    [FILE_MESSAGE_START_GAME_WITH]             filemenu_msg_25,
    [FILE_MESSAGE_FILE_26]                     filemenu_msg_26,
    [FILE_MESSAGE_HAS_BEEN_DELETED]            filemenu_msg_27,
    [FILE_MESSAGE_28]                          filemenu_msg_28,
    [FILE_MESSAGE_COPY_FROM]                   filemenu_msg_29,
    [FILE_MESSAGE_TO]                          filemenu_msg_30,
    [FILE_MESSAGE_HAS_BEEN_CREATED]            filemenu_msg_31,
    [FILE_MESSAGE_ENTER_A_FILE_NAME]           filemenu_msg_32,
    [FILE_MESSAGE_QUESTION]                    filemenu_msg_33,
    [FILE_MESSAGE_PERIOD_34]                   filemenu_msg_34,
    [FILE_MESSAGE_OPTIONS]                     filemenu_msg_options,
    [FILE_MESSAGE_OPT_FPS60]                  filemenu_msg_opt_fps60,
    [FILE_MESSAGE_OPT_WIDE]                  filemenu_msg_opt_wide,
    [FILE_MESSAGE_OPT_HUD]                  filemenu_msg_opt_hud,
    [FILE_MESSAGE_OPT_STATS]                  filemenu_msg_opt_stats,
    [FILE_MESSAGE_OPT_NOFOG]                  filemenu_msg_opt_nofog,
    [FILE_MESSAGE_OPT_NOLIGHT]                  filemenu_msg_opt_nolight,
    [FILE_MESSAGE_OPT_BACK]                  filemenu_msg_opt_back,
    [FILE_MESSAGE_OPT_ON]                  filemenu_msg_opt_on,
    [FILE_MESSAGE_OPT_OFF]                  filemenu_msg_opt_off,
    [FILE_MESSAGE_OPT_LOG]                  filemenu_msg_opt_log,
    [FILE_MESSAGE_OPT_LOGLEVEL]                  filemenu_msg_opt_loglevel,
    [FILE_MESSAGE_OPT_FIXEDSEED]                 filemenu_msg_opt_fixedseed,
    [FILE_MESSAGE_OPT_LV_ERROR]                  filemenu_msg_opt_lv_error,
    [FILE_MESSAGE_OPT_LV_WARN]                  filemenu_msg_opt_lv_warn,
    [FILE_MESSAGE_OPT_LV_INFO]                  filemenu_msg_opt_lv_info,
    [FILE_MESSAGE_OPT_LV_DEBUG]                  filemenu_msg_opt_lv_debug,
};


s32 filemenu_draw_char(s32 c, s32 x, s32 y, s32 flag1, s32 color, s32 flag2) {
    MessageCharset* charset;
    s32 texSizeX;
    s32 texSizeY;
    s32 charRasterSize;
    MessageCharData* charData;
    u8* raster;
    s32 charWidth;

    if (flag1 != filemenu_charset) {
        filemenu_charset = flag1;
        filemenu_charset_raster_id = 0;
    }

    charset = MsgCharsets[filemenu_charset];
    texSizeX = charset->texSize.x;
    texSizeY = charset->texSize.y;
    charRasterSize = charset->charRasterSize;
    charData = &charset->rasters[filemenu_charset_raster_id];
    charWidth = charData->monospaceWidth;
    raster = charData->raster;

    if (c < 0xA2) {
        charWidth = charData->charWidthTable[c];
        if (gMainGfxPos != filemenu_savedGfxPos || nd_rec_pos() != filemenu_savedStreamPos) {
            nd_draw_list(NDN(filemenu_dl_draw_char_init), ND_SELF);
            filemenu_char_color = -1;
        }

        if (filemenu_char_color != color) {
            filemenu_char_color = color;
            nd_tex_load_tlut16(0, D_802F4560[color]);
        }

        if (texSizeX >= 16 && texSizeX % 16 == 0) {
            nd_tex_load_block_4b(&raster[charRasterSize * c], G_IM_FMT_CI, texSizeX, texSizeY, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            nd_tex_load_tile_4b(&raster[charRasterSize * c], G_IM_FMT_CI, texSizeX, texSizeY, 0, 0, texSizeX - 1, texSizeY - 1, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        }

        if (flag2) {
            nd_pso_cc(PM_CC_07, PM_CC_07);
            nd_pso_rm(G_RM_CLD_SURF, G_RM_CLD_SURF2);
            filemenu_draw_rect((x + 2) * 4, (y + 2) * 4, (2 + x + texSizeX) * 4, (2 + y + texSizeY) * 4, 0, 0, 0, 0x400, 0x400);
            nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA);
            nd_pso_rm(G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
        }

        filemenu_draw_rect(x * 4, y * 4, (x + texSizeX) * 4, (y + texSizeY) * 4, 0, 0, 0, 0x400, 0x400);
        filemenu_savedGfxPos = gMainGfxPos;
        filemenu_savedStreamPos = nd_rec_pos();
        return charWidth;
    }


    if (c == MSG_CHAR_READ_SPACE) {
        return charWidth - 6;
    }

    switch (c) {
        case MSG_CHAR_READ_VARIANT0:
            filemenu_charset_raster_id = 0;
            break;
        case MSG_CHAR_READ_VARIANT1:
            filemenu_charset_raster_id = 1;
            break;
        case MSG_CHAR_READ_VARIANT2:
            filemenu_charset_raster_id = 2;
            break;
        case MSG_CHAR_READ_VARIANT3:
            filemenu_charset_raster_id = 3;
            break;
    }
    return 0;
}

void filemenu_draw_message(u8* message, s32 x, s32 y, s32 alpha, s32 color, u32 flags) {
    s32 flag1 = flags & 1;
    s32 flag2 = flags >> 3;

    flag2 &= 1;
    if (flag1 == 1) {
        y -= 2;
    }

    if ((u32)message < 0x100) {
        if ((u32)message - 0xA2 >= 0x4E) {
            filemenu_draw_char((s32)message, x, y, flag1, color, flag2);
            return;
        }
        if ((s32) message >= 0xC6) {
            message = (u8*)filemenu_specialSymbols[(s32) message - 0xC6];
        }
    }

    if (IS_ADDR(message)) {
        u8* tmp;
        filemenu_draw_char(0xF3, x, y, flag1, color, flag2);
        tmp = message;
        while (*tmp != MSG_CHAR_READ_END) {
            x += filemenu_draw_char(*tmp, x, y, flag1, color, flag2);
            tmp++;
        }
    }
}

u8* filemenu_get_menu_message(s32 idx) {
    return gFileMenuMessages[idx];
}

void filemenu_draw_file_name(u8* filename, s32 length, s32 x, s32 y, s32 alpha, s32 arg5, s32 arg6, s32 charWidth) {
    s32 i;

    for (i = 0; i < length; i++) {
        u32 c = filename[i];

        if (c != MSG_CHAR_READ_SPACE) {
            filemenu_draw_message((u8*)c, x + (i * charWidth), y, alpha, arg5, arg6);
        }
    }
}
