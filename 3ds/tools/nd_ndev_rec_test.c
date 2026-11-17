/* NTABFIND host test: nd_ndev_pack (nd_gtab.h) keeps the record-time NdNDev when its owner clears the entry before core 0 runs the record
 * (imgfx.c imgfx_cache_instance_data sets gfxDev[i].list to NULL). Case 0: the copy survives the clear. Case 1: the words gt_run reads. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "nd_gtab.h"

int main(void) {
    static uint32_t buf[8] = { 0x01020304u, 0x05060708u };
    NdNDev e = { buf, 0u, buf, 3u, 2u };
    uint32_t rec[2u * ND_NDEV_REC_UNITS];
    NdNDev d;
    memset(rec, 0, sizeof rec);
    nd_ndev_pack(rec, &e, 0x00123456u);
    e.list = NULL; e.u = NULL; e.nu = 0u; e.ntri = 0u; /* the owner clears the entry (imgfx.c:476) */
    nd_ndev_unpack(&d, rec);
    if (d.list != (const void*)buf || d.u != buf || d.nu != 3u || d.ntri != 2u || d.fl != 0u) { printf("ndev FAIL case 0\n"); return 1; }
    if (rec[1] != 0u || rec[2] != 0x00123456u || rec[3] != 0u) { printf("ndev FAIL case 1\n"); return 1; }
    printf("ndev ok\n");
    return 0;
}
