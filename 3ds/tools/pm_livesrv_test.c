/* CAPLOG host test: pm_live_serve and pm_live_hello (pm_livesrv.h). */
#include <stdio.h>
#include <string.h>
#include "pm_livesrv.h"

int main(void) {
    char b[96];
    if (!pm_live_serve(0, 1)) { printf("caplog FAIL serve logging off\n"); return 1; }
    if (pm_live_serve(0, 0)) { printf("caplog FAIL serve no socket\n"); return 1; }
    if (!pm_live_serve(1, 0)) { printf("caplog FAIL serve queue on\n"); return 1; }
    pm_live_hello(b, sizeof b, 0, 3, 3, 0);
    if (strcmp(b, "#PMLIVE log_on 0 live 3 disk 3 prof 0\n")) { printf("caplog FAIL hello [%s]\n", b); return 1; }
    printf("caplog ok\n");
    return 0;
}
