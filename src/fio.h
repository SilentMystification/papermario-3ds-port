#ifndef _FIO_H_
#define _FIO_H_

#include "common.h"

bool fio_load_globals(void);
bool fio_save_globals(void);
bool fio_load_game(s32 saveSlot);
void fio_save_game(s32 saveSlot);
void fio_erase_game(s32 saveSlot);
#if VERSION_PAL
bool func_PAL_8002B574(void);
#endif

extern SaveMetadata gSaveSlotMetadata[4];
extern u8 gSaveSlotHasData[4];
extern SaveGlobals gSaveGlobals;

#ifdef TARGET_3DS
void pm_cheat_file_copy(s32 from, s32 to); // port (pm_cheatsave.c): Copy File copies the cheat file of the source slot
#endif

#endif
