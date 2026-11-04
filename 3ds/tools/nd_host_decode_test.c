/* nd_host_decode_test.c: decoder check. Cull write, an incremental write of three texture words (odd count, one padding word),
 * a texture address write, NUMVERTICES 9 and one DRAWELEMENTS trigger. Expect one draw line with cull 2 and tris 3. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define ND_DECODE_NO_MAIN
#include "nd_pica_decode.c"
int main(void) {
    uint32_t p[] = {
        0x00000040u, 2u, 0u,                          /* FACECULLING_CONFIG = 2, count 1, padding */
        0x80000082u | (2u << 20), 0x11u, 0x22u, 0x33u, 0u, /* incremental: DIM, PARAM, LOD; count 3, padding */
        0x00000085u, 0x44u, 0u,                       /* TEXUNIT0_ADDR1 */
        0x00000228u, 9u, 0u,                          /* NUMVERTICES 9 */
        0x0000022Fu, 1u, 0u,                          /* DRAWELEMENTS */
    };
    FILE* f = tmpfile();
    int d = nd_decode_words(p, sizeof p / sizeof p[0], f);
    rewind(f);
    char line[256] = {0};
    if (fgets(line, sizeof line, f) == NULL) line[0] = 0;
    int ok = d == 1 && strstr(line, "cull 2") && strstr(line, "tris 3") && strstr(line, "tw0 00000011 00000022 00000033 00000044");
    printf("%s decoder draws %d line: %s", ok ? "PASS" : "FAIL", d, line);
    return ok ? 0 : 1;
}
