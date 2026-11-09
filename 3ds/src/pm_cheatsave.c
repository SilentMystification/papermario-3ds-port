/* CHEATSAVE: the persistent cheat switches of the Select menu Cheats tab, one file per save slot (cheats_slot<N>.ini in the app folder, next to
 * options.ini). Nothing goes into save.flash. The set is bits 0 to 5 (the pm_cheat_get indices of pm_dbg_game.c: infinite HP, infinite FP, infinite
 * star power, no battles, speed x2, perfect action commands) and bit 6 (moon jump). Not in the set: frame advance (a debug hold, it would freeze a
 * new game), instant win (an action), the equipment action commands row (game state, already in the save file). Dirty: the set differs from the
 * set loaded or saved last. Load: pm_cheat_enter_world at each entry into the world (pm_map.c), from the file of the save slot in use; a new file or a
 * warp without a save file gets no file (slot -1). Boot switches (cheats=, moonjump) OR into the loaded set and do not mark dirty. */
#include "common.h"
#include "functions.h"
#include "game_modes.h"
#include "fio.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CK_N 7
static const char* const ck_name[CK_N] = { "hp", "fp", "sp", "nobattle", "speed", "action", "moonjump" };
unsigned pm_cheat_persist_get(void); void pm_cheat_persist_set(unsigned bits); unsigned pm_cheat_boot_bits(void); /* pm_dbg_game.c */
int pm_warp_take_nofile(void); /* pm_warp.c: 1 when this entry is a warp without a save file (consumed) */

static int slot_now = -1; /* the save slot whose file is applied: -1 none (a new file, a warp without a file) */
static unsigned saved_bits; /* the set as loaded or saved last */

static int count_on(unsigned b) { int n = 0; for (int k = 0; k < CK_N; k++) n += (int)((b >> k) & 1u); return n; }
static void file_name(char* out, int slot, const char* ext) { sprintf(out, "cheats_slot%d.%s", slot, ext); }

/* The set in the file of slot (0 when there is no file). found: 1 when the file exists. Keys absent or not 1 are off. */
static unsigned read_file(int slot, int* found) {
    char nm[24], line[32];
    unsigned bits = 0;
    FILE* f;
    *found = 0;
    file_name(nm, slot, "ini");
    f = fopen(nm, "r");
    if (!f) return 0;
    *found = 1;
    while (fgets(line, sizeof line, f)) {
        char* eq = strchr(line, '=');
        int k;
        if (line[0] == ';' || !eq) continue;
        *eq = 0;
        for (k = 0; k < CK_N && strcmp(line, ck_name[k]); k++) {}
        if (k < CK_N && eq[1] == '1') bits |= 1u << k;
    }
    fclose(f);
    return bits;
}

/* Called at each entry into the world (pm_map.c, GAME_MODE_ENTER_WORLD). The save slot is the one the file select or the warp loaded.
 * Slot -1 in the log line: no save file (a new file, a warp without a file). */
void pm_cheat_enter_world(void) {
    int nofile = pm_warp_take_nofile(), found = 0, slot = nofile ? -1 : (int)gGameStatusPtr->saveSlot;
    unsigned bits = slot >= 0 ? read_file(slot, &found) : 0;
    bits |= pm_cheat_boot_bits();
    slot_now = slot;
    saved_bits = bits;
    pm_cheat_persist_set(bits);
    pm_log("cheats: loaded slot %d (%d on%s)", slot, count_on(bits), found ? "" : ", no file");
}

/* The footer of the Cheats tab shows while the set differs and a save slot is in use. */
int pm_cheat_dirty(void) { return slot_now >= 0 && pm_cheat_persist_get() != saved_bits; }
int pm_cheat_slot(void) { return slot_now; }

/* Write bits to the file of slot (temp, then rename, as options.ini). 1 when written. */
static int write_file(int slot, unsigned bits) {
    char tmp[24], fin[24];
    FILE* f;
    int k;
    file_name(tmp, slot, "tmp");
    file_name(fin, slot, "ini");
    f = fopen(tmp, "w");
    for (k = 0; f && k < CK_N; k++) fprintf(f, "%s=%d\n", ck_name[k], (int)((bits >> k) & 1u));
    if (f && fclose(f) == 0) {
        unlink(fin); /* FAT rename does not replace an existing file (options.ini does the same) */
        if (rename(tmp, fin) == 0) {
            pm_log("cheats: saved slot %d (%d on)", slot, count_on(bits));
            return 1;
        }
    }
    pm_log_e("cheats: write of %s failed", fin);
    return 0;
}

/* X on the Cheats tab: write the set to the slot file. 1 when written. */
int pm_cheat_save(void) {
    unsigned bits = pm_cheat_persist_get();
    if (slot_now < 0 || !write_file(slot_now, bits)) return 0;
    saved_bits = bits;
    return 1;
}

/* fio_erase_game (fio.c): a deleted save drops its cheat file, so a new file in that slot starts with all off. */
void pm_cheat_file_erase(int slot) {
    char nm[24];
    file_name(nm, slot, "ini");
    unlink(nm);
}

/* File select Copy File (filemenu_yesno.c): the destination slot gets the cheat file of the source slot, or none when the source has none. */
void pm_cheat_file_copy(s32 from, s32 to) {
    int found;
    unsigned bits = read_file(from, &found);
    if (found) write_file(to, bits);
    else pm_cheat_file_erase(to);
}
