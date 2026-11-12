#ifndef _H_INCLUDE_ASSET
/* S7CONV host copy of 3ds/stub_include/include_asset.h for the gfx table tool (gen_gfx_tab.sh, x86 gcc -m32): x86 gas reads `.align 3` as
 * 3 bytes, so the data is aligned with .balign 8; a <NAME>_gend label after the incbin gives the image size (the tool's symbol map). */
#define _H_INCLUDE_ASSET
#define ASTRINGIFY_(x) #x
#define ASTRINGIFY(x) ASTRINGIFY_(x)

#define GCONV_INCBIN(SYMBOLNAME, PATH) \
    __asm__( \
        ".globl " #SYMBOLNAME "\n" \
        ".globl " #SYMBOLNAME "_gend\n" \
        ".pushsection .data\n" \
        ".balign 8\n" \
        ".type " #SYMBOLNAME ", @object\n" \
        #SYMBOLNAME ":\n" \
        ".incbin \"" PATH "\"\n" \
        #SYMBOLNAME "_gend:\n" \
        ".popsection\n" \
    )

#define _INCLUDE_IMG(FILENAME, SYMBOLNAME) \
    extern unsigned char SYMBOLNAME[]; \
    GCONV_INCBIN(SYMBOLNAME, "ver/" ASTRINGIFY(VERSION) "/build/" FILENAME ".bin")
#define INCLUDE_IMG(FILENAME, SYMBOLNAME) _INCLUDE_IMG(FILENAME, SYMBOLNAME)
#define INCLUDE_PAL(FILENAME, SYMBOLNAME) \
    extern unsigned short SYMBOLNAME[]; \
    GCONV_INCBIN(SYMBOLNAME, "ver/" ASTRINGIFY(VERSION) "/build/" FILENAME ".bin")
#define INCLUDE_RAW(FILENAME, SYMBOLNAME) \
    extern unsigned char SYMBOLNAME[]; \
    GCONV_INCBIN(SYMBOLNAME, "ver/" ASTRINGIFY(VERSION) "/build/assets/" ASTRINGIFY(VERSION) "/" FILENAME)
#endif
