#include "common.h"
#include "filemenu.h"
#include "game_modes.h"

// port: Options panel of the file select screen (F11-3). TARGET_3DS only.
// UI1a: three tabs (General, Controls, Debug), L and R change the tab. The rows, values and actions come from the port layer (pm_options.c, pm_optui_*).
#ifdef TARGET_3DS

#define OPT_ROWS 22 // longest tab (Controls, 20 rows) with spare
#define OPT_TABS 3
#define OPT_VIS 10 // rows shown at once

static s32 optTab;
static s32 optTop;

u8 filemenu_options_gridData[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
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
    gWindows[WIN_FILES_OPTIONS_PANEL].width = 220;
    gWindows[WIN_FILES_OPTIONS_PANEL].height = 214;
    gWindows[WIN_FILES_OPTIONS_PANEL].pos.x = CENTER_WINDOW_X(WIN_FILES_OPTIONS_PANEL);
    gWindows[WIN_FILES_OPTIONS_PANEL].pos.y = CENTER_WINDOW_Y(WIN_FILES_OPTIONS_PANEL);
    menu->state = 0;
    optTab = 0;
    optTop = 0;
    menu->numRows = pm_optui_rows(0);
    pm_pad_take_tab();
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
    static const char* tabNames[OPT_TABS] = { "General", "Controls", "Debug" };
    static const s32 tabX[OPT_TABS] = { 12, 76, 150 };
    s32 rows = pm_optui_rows(optTab);
    s32 i;

    // UI1a: tab strip (L and R change the tab), the active tab has the second palette
    for (i = 0; i < OPT_TABS; i++) {
        filemenu_draw_message((u8*)pm_options_msg(tabNames[i], 4 + i), baseX + tabX[i], baseY + 8, 255, i == optTab ? 4 : 0, 1);
    }
    if (optTop > 0) {
        filemenu_draw_message((u8*)pm_options_msg("-", 3), baseX + 6, baseY + 8 + 17, 255, 0, 1);
    }
    for (i = optTop; i < rows && i < optTop + OPT_VIS; i++) {
        s32 y = baseY + 8 + 17 * (1 + i - optTop);
        const char* value = pm_optui_value(optTab, i);

        filemenu_draw_message((u8*)pm_options_msg(pm_optui_label(optTab, i), 0), baseX + 24, y, 255, 0, 1);
        if (value != nullptr) {
            filemenu_draw_message((u8*)pm_options_msg(value, 1), baseX + 120, y, 255, 0, 1);
        }
        if (panel->row == i) {
            filemenu_set_cursor_goal_pos(WIN_FILES_OPTIONS_PANEL, baseX + 12, y + 8);
        }
    }
    filemenu_draw_message((u8*)pm_options_build_msg(), baseX + 24, baseY + 8 + 17 * (OPT_VIS + 1), 255, 0, 1);
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
    // F11-9: the panel moved one row per stick jitter. Gate the direction: the pad must be pushed to 56 of 80 (the game dead zone and
    // button threshold are 32), a d-pad press always counts, and two moves are at least 6 updates apart (the held repeat is 15 then 4).
    static s32 moveGap;
    s32 sy = gGameStatusPtr->stickY[0];
    s32 held = filemenu_heldButtons;
    s32 up = (held & BUTTON_D_UP) || ((held & BUTTON_STICK_UP) && sy >= 56);
    s32 down = (held & BUTTON_D_DOWN) || ((held & BUTTON_STICK_DOWN) && sy <= -56);
    s32 tabMove = pm_pad_take_tab();

    if (filemenu_pressedButtons & BUTTON_L) {
        tabMove = -1;
    } else if (filemenu_pressedButtons & BUTTON_R) {
        tabMove = 1;
    }
    if (tabMove != 0) {
        // UI1a: L and R change the section
        optTab = (optTab + tabMove + OPT_TABS) % OPT_TABS;
        optTop = 0;
        menu->row = 0;
        menu->numRows = pm_optui_rows(optTab);
        filemenu_set_selected(menu, 0, 0);
        pm_log_d("options: tab %d", optTab);
        sfx_play_sound(SOUND_MENU_CHANGE_SELECTION);
        return;
    }

    if (moveGap > 0) {
        moveGap--;
    }
    if (moveGap == 0 && (up || down)) {
        menu->row += down ? 1 : -1;
        moveGap = 6;
        if (menu->row < 0) {
            menu->row = 0;
        }
        if (menu->row >= menu->numRows) {
            menu->row = menu->numRows - 1;
        }
    }
    if (oldRow != menu->row) {
        filemenu_set_selected(menu, 0, menu->row);
        if (menu->row < optTop) {
            optTop = menu->row;
        }
        if (menu->row >= optTop + OPT_VIS) {
            optTop = menu->row - OPT_VIS + 1;
        }
        pm_log_d("options: row %d", menu->row);
        sfx_play_sound(SOUND_MENU_CHANGE_SELECTION);
    }

    {   // PT01: left and right change a value (Warp to, Entry). Press steps once, hold repeats (15 updates, then every 4, 5 maps a step after 12 repeats).
        static s32 lrGap, lrRep;
        s32 sx = gGameStatusPtr->stickX[0];
        s32 lr = ((held & BUTTON_D_RIGHT) || ((held & BUTTON_STICK_RIGHT) && sx >= 56)) ? 1 : ((held & BUTTON_D_LEFT) || ((held & BUTTON_STICK_LEFT) && sx <= -56)) ? -1 : 0;

        if (lr == 0) {
            lrGap = 0;
            lrRep = 0;
        } else if (lrGap > 0) {
            lrGap--;
        } else {
            lrGap = lrRep == 0 ? 15 : 4;
            lrRep++;
            if (pm_optui_step(optTab, menu->row, lr * (lrRep > 12 ? 5 : 1))) {
                sfx_play_sound(SOUND_MENU_CHANGE_SELECTION);
            }
        }
    }
    if (filemenu_pressedButtons & BUTTON_B) {
        filemenu_options_close();
    } else if (filemenu_pressedButtons & BUTTON_A) {
        s32 result;

        sfx_play_sound(SOUND_MENU_NEXT);
        result = pm_optui_act(optTab, menu->selected);
        if (result == 1) {
            filemenu_options_close();
        } else if (result == 2) {
            // Warp now: leave the file select as the Cancel button does, the exit mode of the file select is set by pm_warp_pending
            set_window_update(WIN_FILES_TITLE, (s32)filemenu_update_hidden_title);
            set_window_update(WIN_FILES_OPTIONS_PANEL, WINDOW_UPDATE_HIDE);
            sfx_play_sound(SOUND_FILE_MENU_OUT);
            set_game_mode(GAME_MODE_END_FILE_SELECT);
        }
    }
}

void filemenu_options_update(MenuPanel* menu) {
}

void filemenu_options_cleanup(MenuPanel* menu) {
}
#endif
