/* The RSP audio microcode does not exist on the 3DS. The aXxx packet macros run the work at once in pm_rsp_audio.c. */
#ifndef PM_ABI_SHADOW_H
#define PM_ABI_SHADOW_H
#include_next <PR/abi.h>
#include "pm_rsp_audio.h"
#endif
