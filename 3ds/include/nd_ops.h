/* nd_ops.h: F3DEX2 opcode numbers for the native code (include/PR/gbi.h values; nd_gbi_test.c checks them). */
#ifndef ND_OPS_H
#define ND_OPS_H
#define OP_NOOP 0x00
#define OP_VTX 0x01
#define OP_TRI1 0x05
#define OP_TRI2 0x06
#define OP_QUAD 0x07
#define OP_TEXTURE 0xD7
#define OP_POPMTX 0xD8
#define OP_GEOMETRYMODE 0xD9
#define OP_MTX 0xDA
#define OP_MOVEWORD 0xDB
#define OP_MOVEMEM 0xDC
#define OP_DL 0xDE
#define OP_ENDDL 0xDF
#define OP_RDPHALF_1 0xE1
#define OP_SETOTHERMODE_L 0xE2
#define OP_SETOTHERMODE_H 0xE3
#define OP_TEXRECT 0xE4
#define OP_TEXRECTFLIP 0xE5
#define OP_LOADSYNC 0xE6
#define OP_PIPESYNC 0xE7
#define OP_TILESYNC 0xE8
#define OP_FULLSYNC 0xE9
#define OP_SETKEYGB 0xEA
#define OP_SETKEYR 0xEB
#define OP_SETCONVERT 0xEC
#define OP_SETSCISSOR 0xED
#define OP_SETPRIMDEPTH 0xEE
#define OP_RDPSETOTHERMODE 0xEF
#define OP_LOADTLUT 0xF0
#define OP_RDPHALF_2 0xF1
#define OP_SETTILESIZE 0xF2
#define OP_LOADBLOCK 0xF3
#define OP_LOADTILE 0xF4
#define OP_SETTILE 0xF5
#define OP_FILLRECT 0xF6
#define OP_SETFILLCOLOR 0xF7
#define OP_SETFOGCOLOR 0xF8
#define OP_SETBLENDCOLOR 0xF9
#define OP_SETPRIMCOLOR 0xFA
#define OP_SETENVCOLOR 0xFB
#define OP_SETCOMBINE 0xFC
#define OP_SETTIMG 0xFD
#define OP_SETZIMG 0xFE
#define OP_SETCIMG 0xFF
#endif
