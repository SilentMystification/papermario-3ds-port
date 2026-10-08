#include <3ds.h>
#include <string.h>
#include "pm_port.h"

#define CAP 32768

static int ready;
static unsigned rate = 32000;
static ndspWaveBuf wb[2];
static s16* data[2];

void pm_audio_init(void) {
    if (ready) return;
    if (R_FAILED(ndspInit())) {
        pm_log("ndsp init failed\n");
        return;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(0, (float)rate);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    for (int i = 0; i < 2; i++) {
        data[i] = (s16*)linearAlloc(CAP);
        memset(&wb[i], 0, sizeof(wb[i]));
        wb[i].status = NDSP_WBUF_DONE;
    }
    ready = 1;
    pm_log("ndsp ready %u Hz\n", rate);
}

void pm_audio_post(const void* pcm, unsigned bytes, unsigned hz) {
    if (hz && hz != rate) {
        rate = hz;
        if (ready) ndspChnSetRate(0, (float)rate);
    }
    if (!pcm || bytes < 4) return;
    if (!ready) pm_audio_init();
    if (!ready) return;
    int slot = -1;
    for (int i = 0; i < 2; i++) {
        if (wb[i].status != NDSP_WBUF_QUEUED && wb[i].status != NDSP_WBUF_PLAYING) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;
    if (bytes > CAP) bytes = CAP;
    bytes &= ~3u;
    memcpy(data[slot], pcm, bytes);
    DSP_FlushDataCache(data[slot], bytes);
    memset(&wb[slot], 0, sizeof(wb[slot]));
    wb[slot].data_vaddr = data[slot];
    wb[slot].nsamples = bytes / 4;
    wb[slot].status = NDSP_WBUF_FREE;
    ndspChnWaveBufAdd(0, &wb[slot]);
}
