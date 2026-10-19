#include "common.h"
#include "filemenu.h"

// port: Options panel of the file select screen (F11-3). TARGET_3DS only.
// Rows 0 to 5 are settings (60 FPS, Widescreen, HUD, Stats, No fog, No light). Row 6 is Logging (On or Off), row 7 is Log level (Error, Warn, Info, Debug), row 8 is Back.
// The panel holds no setting logic. It reads and flips the values in the port layer (pm_options.c).
#ifdef TARGET_3DS

#define OPT_ROWS 9
#define OPT_ROW_BACK 8

u8 filemenu_options_gridData[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8
};

MenuWindowBP filemenu_options_windowBPs[] = {
    {
        .windowID = WIN_FILES_OPTIONS_PANEL,
        .unk_01 = 0,
        .pos = { .x = 0, .y = 0 },
        .width = 0,
        .height = 0,
        .priority = WINDOW_PRIORITY_64,
        .fpDrawContents = &filemenu_draw_contents_options_panel,
        .tab = nullptr,
        .parentID = -1,
        .fpUpdate = { WINDOW_UPDATE_HIDE },
        .extraFlags = 0,
        .style = { .customStyle = &filemenu_windowStyles[19] }
    },
};

MenuPanel filemenu_options_menuBP = {
    .initialized = false,
    .col = 0,
    .row = 0,
    .selected = 0,
    .state = 0,
    .numCols = 1,
    .numRows = OPT_ROWS,
    .numPages = 0,
    .gridData = filemenu_options_gridData,
    .fpInit = &filemenu_options_init,
    .fpHandleInput = &filemenu_options_handle_input,
    .fpUpdate = &filemenu_options_update,
    .fpCleanup = &filemenu_options_cleanup
};

void filemenu_options_open(void) {
    MenuPanel* menu = &filemenu_options_menuBP;

    sfx_play_sound(SOUND_MENU_NEXT);
    gWindows[WIN_FILES_OPTIONS_PANEL].width = 200;
    gWindows[WIN_FILES_OPTIONS_PANEL].height = 176;
    gWindows[WIN_FILES_OPTIONS_PANEL].pos.x = CENTER_WINDOW_X(WIN_FILES_OPTIONS_PANEL);
    gWindows[WIN_FILES_OPTIONS_PANEL].pos.y = CENTER_WINDOW_Y(WIN_FILES_OPTIONS_PANEL);
    menu->state = 0;
    filemenu_set_selected(menu, 0, 0);
    set_window_update(WIN_FILES_STEREO, (s32)filemenu_update_hidden_options_left);
    set_window_update(WIN_FILES_MONO, (s32)filemenu_update_hidden_options_right);
    set_window_update(WIN_FILES_OPTION_LEFT, (s32)filemenu_update_hidden_options_bottom);
    set_window_update(WIN_FILES_OPTION_CENTER, (s32)filemenu_update_hidden_options_bottom);
    set_window_update(WIN_FILES_OPTION_RIGHT, (s32)filemenu_update_hidden_options_bottom);
    set_window_update(WIN_FILES_SLOT1_BODY, (s32)filemenu_update_hidden_options_left);
    set_window_update(WIN_FILES_SLOT2_BODY, (s32)filemenu_update_hidden_options_right);
    set_window_update(WIN_FILES_SLOT3_BODY, (s32)filemenu_update_hidden_options_left);
    set_window_update(WIN_FILES_SLOT4_BODY, (s32)filemenu_update_hidden_options_right);
    filemenu_currentMenu = FILE_MENU_OPTIONS;
    set_window_update(WIN_FILES_OPTIONS_PANEL, WINDOW_UPDATE_SHOW);
}

static void filemenu_options_close(void) {
    sfx_play_sound(SOUND_MENU_BACK);
    pm_options_close(); // writes options.txt if a value changed
    filemenu_currentMenu = FILE_MENU_MAIN;
    set_window_update(WIN_FILES_STEREO, (s32)filemenu_update_show_options_left);
    set_window_update(WIN_FILES_MONO, (s32)filemenu_update_show_options_right);
    set_window_update(WIN_FILES_OPTION_LEFT, (s32)filemenu_update_show_options_bottom);
    set_window_update(WIN_FILES_OPTION_CENTER, (s32)filemenu_update_show_options_bottom);
    set_window_update(WIN_FILES_OPTION_RIGHT, (s32)filemenu_update_show_options_bottom);
    set_window_update(WIN_FILES_SLOT1_BODY, (s32)filemenu_update_show_options_left);
    set_window_update(WIN_FILES_SLOT2_BODY, (s32)filemenu_update_show_options_right);
    set_window_update(WIN_FILES_SLOT3_BODY, (s32)filemenu_update_show_options_left);
    set_window_update(WIN_FILES_SLOT4_BODY, (s32)filemenu_update_show_options_right);
    set_window_update(WIN_FILES_OPTIONS_PANEL, WINDOW_UPDATE_HIDE);
}

void filemenu_draw_contents_options_panel(
    MenuPanel* menu,
    s32 baseX, s32 baseY,
    s32 width, s32 height,
    s32 opacity, s32 darkening
) {
    MenuPanel* panel = &filemenu_options_menuBP;
    static const s32 labels[OPT_ROWS] = {
        FILE_MESSAGE_OPT_FPS60, FILE_MESSAGE_OPT_WIDE, FILE_MESSAGE_OPT_HUD, FILE_MESSAGE_OPT_STATS,
        FILE_MESSAGE_OPT_NOFOG, FILE_MESSAGE_OPT_NOLIGHT, FILE_MESSAGE_OPT_LOG, FILE_MESSAGE_OPT_LOGLEVEL, FILE_MESSAGE_OPT_BACK
    };
    s32 i;

    for (i = 0; i < OPT_ROWS; i++) {
        s32 y = baseY + 8 + i * 17;

        filemenu_draw_message(filemenu_get_menu_message(labels[i]), baseX + 24, y, 255, 0, 1);
        if (i != OPT_ROW_BACK) {
            static const s32 levelMsgs[4] = {
                FILE_MESSAGE_OPT_LV_ERROR, FILE_MESSAGE_OPT_LV_WARN, FILE_MESSAGE_OPT_LV_INFO, FILE_MESSAGE_OPT_LV_DEBUG
            };
            s32 msg = i == 7 ? levelMsgs[pm_options_get(i) & 3] : pm_options_get(i) ? FILE_MESSAGE_OPT_ON : FILE_MESSAGE_OPT_OFF;

            filemenu_draw_message(filemenu_get_menu_message(msg), baseX + 140, y, 255, 0, 1);
        }
        if (panel->row == i) {
            filemenu_set_cursor_goal_pos(WIN_FILES_OPTIONS_PANEL, baseX + 12, y + 8);
        }
    }
}

void filemenu_options_init(MenuPanel* tab) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(filemenu_options_windowBPs); i++) {
        filemenu_options_windowBPs[i].tab = tab;
    }

    setup_pause_menu_tab(filemenu_options_windowBPs, ARRAY_COUNT(filemenu_options_windowBPs));
    tab->initialized = true;
}

void filemenu_options_handle_input(MenuPanel* menu) {
    s32 oldRow = menu->row;

    if (filemenu_heldButtons & BUTTON_STICK_UP) {
        menu->row--;
        if (menu->row < 0) {
            menu->row = 0;
        }
    }
    if (filemenu_heldButtons & BUTTON_STICK_DOWN) {
        menu->row++;
        if (menu->row >= menu->numRows) {
            menu->row = menu->numRows - 1;
        }
    }
    if (oldRow != menu->row) {
        filemenu_set_selected(menu, 0, menu->row);
        sfx_play_sound(SOUND_MENU_CHANGE_SELECTION);
    }

    if (filemenu_pressedButtons & BUTTON_B) {
        filemenu_options_close();
    } else if (filemenu_pressedButtons & BUTTON_A) {
        if (menu->selected == OPT_ROW_BACK) {
            filemenu_options_close();
        } else {
            sfx_play_sound(SOUND_MENU_NEXT);
            pm_options_flip(menu->selected);
        }
    }
}

void filemenu_options_update(MenuPanel* menu) {
}

void filemenu_options_cleanup(MenuPanel* menu) {
}
#endif
