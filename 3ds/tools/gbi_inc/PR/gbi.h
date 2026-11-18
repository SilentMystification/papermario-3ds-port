/* gbi_inc/PR/gbi.h (DEL4D1, written by 3ds/tools/gbimove.py): the gbi macros for the host tools only (gconv, host tests). */
#ifndef ND_GBI_INC_H
#define ND_GBI_INC_H
#include_next <PR/gbi.h>
#ifdef    F3DEX_GBI_2
# ifndef  F3DEX_GBI
# endif
#else	/* F3DEX_GBI_2 */
#if (defined(F3DEX_GBI)||defined(F3DLP_GBI))
#else
#endif
#endif	/* F3DEX_GBI_2 */
#ifdef _LANGUAGE_ASSEMBLY
#endif
#ifdef	F3DEX_GBI_2
#else	/* F3DEX_GBI_2 */
#endif	/* F3DEX_GBI_2 */
#ifdef	F3DEX_GBI_2
#else
#endif
#if	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
#else
#endif
#ifdef _LANGUAGE_ASSEMBLY
#if	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
#endif
#endif
#ifdef _LANGUAGE_ASSEMBLY
#endif
#ifndef _HW_VERSION_1
#else
#endif
#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)
#ifdef	F3DEX_GBI_2
#else	/* F3DEX_GBI_2 */
#endif	/* F3DEX_GBI_2 */
#ifdef	F3DEX_GBI_2
#else	/* F3DEX_GBI_2 */
#endif	/* F3DEX_GBI_2 */
#ifdef	F3DEX_GBI_2
#else
#endif
#define	gDma0p(pkt, c, s, l)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL((c), 24, 8) | _SHIFTL((l), 0, 24);	\
	_g->words.w1 = (unsigned int)(s);				\
}
#define	gDma1p(pkt, c, s, l, p)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) |	\
			_SHIFTL((l), 0, 16));				\
	_g->words.w1 = (unsigned int)(s);				\
}
#define	gDma2p(pkt, c, adrs, len, idx, ofs)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL((c),24,8)|_SHIFTL(((len)-1)/8,19,5)|	\
			_SHIFTL((ofs)/8,8,8)|_SHIFTL((idx),0,8));	\
	_g->words.w1 = (unsigned int)(adrs);				\
}
#define	gSPNoOp(pkt)		gDma0p(pkt, G_SPNOOP, 0, 0)
#ifdef	F3DEX_GBI_2
# define	gSPMatrix(pkt, m, p)	\
		gDma2p((pkt),G_MTX,(m),sizeof(Mtx),(p)^G_MTX_PUSH,0)
# define	gsSPMatrix(m, p)	\
		gsDma2p(     G_MTX,(m),sizeof(Mtx),(p)^G_MTX_PUSH,0)
#else	/* F3DEX_GBI_2 */
# define	gSPMatrix(pkt, m, p)	gDma1p(pkt, G_MTX, m, sizeof(Mtx), p)
# define	gsSPMatrix(m, p)	gsDma1p(G_MTX, m, sizeof(Mtx), p)
#endif	/* F3DEX_GBI_2 */
#if	defined(F3DEX_GBI_2)
# define	gSPVertex(pkt, v, n, v0)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 =							\
	  _SHIFTL(G_VTX,24,8)|_SHIFTL((n),12,8)|_SHIFTL((v0)+(n),1,7);	\
	_g->words.w1 = (unsigned int)(v);				\
}
# define	gsSPVertex(v, n, v0)					\
{{									\
	(_SHIFTL(G_VTX,24,8)|_SHIFTL((n),12,8)|_SHIFTL((v0)+(n),1,7)),	\
        (unsigned int)(v)						\
}}
#elif	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
# define	gSPVertex(pkt, v, n, v0) \
                gDma1p((pkt),G_VTX,(v),((n)<<10)|(sizeof(Vtx)*(n)-1),(v0)*2)
# define	gsSPVertex(v, n, v0) \
                gsDma1p(G_VTX,(v),((n)<<10)|(sizeof(Vtx)*(n)-1),(v0)*2)
#else
# define	gSPVertex(pkt, v, n, v0) \
                gDma1p(pkt, G_VTX, v, sizeof(Vtx)*(n),((n)-1)<<4|(v0))
# define	gsSPVertex(v, n, v0) \
                gsDma1p(G_VTX, v, sizeof(Vtx)*(n), ((n)-1)<<4|(v0))
#endif
#ifdef	F3DEX_GBI_2
# define gSPViewport(pkt, v)	\
		gDma2p((pkt), G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT, 0)
# define gsSPViewport(v)	\
		gsDma2p(      G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT, 0)
#else	/* F3DEX_GBI_2 */
# define gSPViewport(pkt,v)	\
		gDma1p((pkt), G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT)
# define gsSPViewport(v)	\
		gsDma1p(      G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT)
#endif	/* F3DEX_GBI_2 */
#define	gSPDisplayList(pkt,dl)	gDma1p(pkt,G_DL,dl,0,G_DL_PUSH)
#define	gsSPDisplayList(   dl)	gsDma1p(   G_DL,dl,0,G_DL_PUSH)
#define	gSPBranchList(pkt,dl)	gDma1p(pkt,G_DL,dl,0,G_DL_NOPUSH)
#define	gsSPBranchList(   dl)	gsDma1p(   G_DL,dl,0,G_DL_NOPUSH)
#define	gImmp0(pkt, c)							\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL((c), 24, 8);				\
}
#define	gImmp1(pkt, c, p0)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL((c), 24, 8);				\
	_g->words.w1 = (unsigned int)(p0);				\
}
#define	gImmp2(pkt, c, p0, p1)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL((c), 24, 8);				\
	_g->words.w1 = _SHIFTL((p0), 16, 16) | _SHIFTL((p1), 8, 8);	\
}
#define	gImmp3(pkt, c, p0, p1, p2)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL((c), 24, 8);				\
	_g->words.w1 = (_SHIFTL((p0), 16, 16) | _SHIFTL((p1), 8, 8) |	\
			_SHIFTL((p2), 0, 8));				\
}
#define	gImmp21(pkt, c, p0, p1, dat)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL((c), 24, 8)  | _SHIFTL((p0), 8, 16) |	\
			_SHIFTL((p1), 0, 8));				\
	_g->words.w1 = (unsigned int) (dat);				\
}
#ifdef	F3DEX_GBI_2
#else	/* F3DEX_GBI_2 */
#endif	/* F3DEX_GBI_2 */
#if	(defined(F3DLP_GBI)||defined(F3DEX_GBI))
#else
#endif
#ifdef	F3DEX_GBI_2
#define gSP1Triangle(pkt, v0, v1, v2, flag)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_TRI1, 24, 8)|				\
			__gsSP1Triangle_w1f(v0, v1, v2, flag);		\
	_g->words.w1 = 0;						\
}
#define gsSP1Triangle(v0, v1, v2, flag)					\
{{									\
	_SHIFTL(G_TRI1, 24, 8)|__gsSP1Triangle_w1f(v0, v1, v2, flag),	\
	0								\
}}
#else	/* F3DEX_GBI_2 */
#define gSP1Triangle(pkt, v0, v1, v2, flag)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_TRI1, 24, 8);				\
	_g->words.w1 = __gsSP1Triangle_w1f(v0, v1, v2, flag);		\
}
#define gsSP1Triangle(v0, v1, v2, flag)					\
{{									\
	_SHIFTL(G_TRI1, 24, 8),						\
	__gsSP1Triangle_w1f(v0, v1, v2, flag)				\
}}
#endif	/* F3DEX_GBI_2 */
#if	(defined(F3DLP_GBI)||defined(F3DEX_GBI))
#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1)	\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_TRI2, 24, 8)|				\
			__gsSP1Triangle_w1f(v00, v01, v02, flag0));	\
        _g->words.w1 =  __gsSP1Triangle_w1f(v10, v11, v12, flag1); 	\
}
#define gsSP2Triangles(v00, v01, v02, flag0, v10, v11, v12, flag1)	\
{{									\
	(_SHIFTL(G_TRI2, 24, 8)|					\
	 __gsSP1Triangle_w1f(v00, v01, v02, flag0)),			\
	 __gsSP1Triangle_w1f(v10, v11, v12, flag1)			\
}}
#endif	/* F3DEX_GBI/F3DLP_GBI */
#if	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
#else
#endif
#define gSPSegment(pkt, segment, base)					\
	gMoveWd(pkt, G_MW_SEGMENT, (segment)*4, base)
#define gSPClipRatio(pkt, r)						\
{									\
	gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNX, FR_NEG_##r);		\
	gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNY, FR_NEG_##r);		\
	gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPX, FR_POS_##r);		\
	gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPY, FR_POS_##r);		\
}
#ifdef	F3DEX_GBI_2
#else
#endif
#ifdef	F3DEX_GBI_2
#else	/* F3DEX_GBI_2 */
#endif	/* F3DEX_GBI_2 */
#if	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
# define gSPModifyVertex(pkt, vtx, where, val)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL(G_MODIFYVTX,24,8)|			\
		        _SHIFTL((where),16,8)|_SHIFTL((vtx)*2,0,16));	\
	_g->words.w1 = (unsigned int)(val);				\
}
# define gsSPModifyVertex(vtx, where, val)				\
{{									\
	_SHIFTL(G_MODIFYVTX,24,8)|					\
	_SHIFTL((where),16,8)|_SHIFTL((vtx)*2,0,16),			\
	(unsigned int)(val)						\
}}
#else
# define gSPModifyVertex(pkt, vtx, where, val)				\
	 gMoveWd(pkt, G_MW_POINTS, (vtx)*40+(where), val)
# define gsSPModifyVertex(vtx, where, val)				\
	 gsMoveWd(G_MW_POINTS, (vtx)*40+(where), val)
#endif
#if	(defined(F3DEX_GBI)||defined(F3DLP_GBI))
#define	gSPBranchLessZrg(pkt, dl, vtx, zval, near, far, flag, zmin, zmax) \
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = _SHIFTL(G_RDPHALF_1,24,8);			\
	_g->words.w1 = (unsigned int)(dl);				\
	_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL(G_BRANCH_Z,24,8)|			\
		        _SHIFTL((vtx)*5,12,12)|_SHIFTL((vtx)*2,0,12));	\
	_g->words.w1 = G_DEPTOZSrg(zval, near, far, flag, zmin, zmax);	\
}
#define	gSPBranchLessZ(pkt, dl, vtx, zval, near, far, flag)		\
	gSPBranchLessZrg(pkt, dl, vtx, zval, near, far, flag, 0, G_MAXZ)
#define	gSPBranchLessZraw(pkt, dl, vtx, zval)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = _SHIFTL(G_RDPHALF_1,24,8);			\
	_g->words.w1 = (unsigned int)(dl);				\
	_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL(G_BRANCH_Z,24,8)|			\
		        _SHIFTL((vtx)*5,12,12)|_SHIFTL((vtx)*2,0,12));	\
	_g->words.w1 = (unsigned int)(zval);				\
}
#define	gSPLoadUcodeEx(pkt, uc_start, uc_dstart, uc_dsize)		\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = _SHIFTL(G_RDPHALF_1,24,8);			\
	_g->words.w1 = (unsigned int)(uc_dstart);			\
	_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL(G_LOAD_UCODE,24,8)|			\
			_SHIFTL((int)(uc_dsize)-1,0,16));		\
	_g->words.w1 = (unsigned int)(uc_start);			\
}
#define	gSPLoadUcode(pkt, uc_start, uc_dstart)				\
        gSPLoadUcodeEx((pkt), (uc_start), (uc_dstart), SP_UCODE_DATA_SIZE)
#endif
#ifdef	F3DEX_GBI_2
#define	gSPDma_io(pkt, flag, dmem, dram, size)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = _SHIFTL(G_DMA_IO,24,8)|_SHIFTL((flag),23,1)|	\
	  _SHIFTL((dmem)/8,13,10)|_SHIFTL((size)-1,0,12);		\
	_g->words.w1 = (unsigned int)(dram);				\
}
#endif
#ifdef	F3DEX_GBI_2
#else
#endif
#define gSPNumLights(pkt, n)						\
	gMoveWd(pkt, G_MW_NUMLIGHT, G_MWO_NUMLIGHT, NUML(n))
#define gsSPNumLights(n)						\
	gsMoveWd(    G_MW_NUMLIGHT, G_MWO_NUMLIGHT, NUML(n))
#ifdef	F3DEX_GBI_2
# define gSPLight(pkt, l, n)	\
	  gDma2p((pkt),G_MOVEMEM,(l),sizeof(Light),G_MV_LIGHT,(n)*24+24)
# define gsSPLight(l, n)	\
	 gsDma2p(      G_MOVEMEM,(l),sizeof(Light),G_MV_LIGHT,(n)*24+24)
#else	/* F3DEX_GBI_2 */
# define gSPLight(pkt, l, n)	\
	 gDma1p(pkt, G_MOVEMEM, l, sizeof(Light),((n)-1)*2+G_MV_L0)
# define gsSPLight(l, n)	\
	 gsDma1p(    G_MOVEMEM, l, sizeof(Light),((n)-1)*2+G_MV_L0)
#endif	/* F3DEX_GBI_2 */
#define gSPLightColor(pkt, n, col)					\
{									\
	gMoveWd(pkt, G_MW_LIGHTCOL, G_MWO_a##n, col);			\
	gMoveWd(pkt, G_MW_LIGHTCOL, G_MWO_b##n, col);			\
}
#define gSPSetLights0(pkt,name)						\
{									\
	gSPNumLights(pkt,NUMLIGHTS_0);					\
	gSPLight(pkt,&name.l[0],1);					\
	gSPLight(pkt,&name.a,2);					\
}
#define gSPSetLights1(pkt,name)						\
{									\
	gSPNumLights(pkt,NUMLIGHTS_1);					\
	gSPLight(pkt,&name.l[0],1);					\
	gSPLight(pkt,&name.a,2);					\
}
#define gsSPSetLights1(name)						\
	gsSPNumLights(NUMLIGHTS_1),					\
	gsSPLight(&name.l[0],1),					\
	gsSPLight(&name.a,2)
#define gSPSetLights2(pkt,name)						\
{									\
	gSPNumLights(pkt,NUMLIGHTS_2);					\
	gSPLight(pkt,&name.l[0],1);					\
	gSPLight(pkt,&name.l[1],2);					\
	gSPLight(pkt,&name.a,3);					\
}
#ifdef	F3DEX_GBI_2
# define gSPLookAtX(pkt, l)	\
	 gDma2p((pkt),G_MOVEMEM,(l),sizeof(Light),G_MV_LIGHT,G_MVO_LOOKATX)
# define gSPLookAtY(pkt, l)	\
	 gDma2p((pkt),G_MOVEMEM,(l),sizeof(Light),G_MV_LIGHT,G_MVO_LOOKATY)
#else	/* F3DEX_GBI_2 */
# define gSPLookAtX(pkt, l)	\
	 gDma1p(pkt, G_MOVEMEM, l, sizeof(Light),G_MV_LOOKATX)
# define gSPLookAtY(pkt, l)	\
	 gDma1p(pkt, G_MOVEMEM, l, sizeof(Light),G_MV_LOOKATY)
#endif	/* F3DEX_GBI_2 */
#define gSPLookAt(pkt, la)						\
{									\
	gSPLookAtX(pkt,la)						\
	gSPLookAtY(pkt,(char *)(la)+16)					\
}
#define gDPSetHilite1Tile(pkt, tile, hilite, width, height)		\
	gDPSetTileSize(pkt, tile, (hilite)->h.x1 & 0xfff, (hilite)->h.y1 & 0xfff, 	\
		((((width)-1)*4)+(hilite)->h.x1) & 0xfff, ((((height)-1)*4)+(hilite)->h.y1) & 0xfff)
#define gDPSetHilite2Tile(pkt, tile, hilite, width, height)		\
	gDPSetTileSize(pkt, tile, (hilite)->h.x2 & 0xfff, (hilite)->h.y2 & 0xfff, 	\
		((((width)-1)*4)+(hilite)->h.x2) & 0xfff, ((((height)-1)*4)+(hilite)->h.y2) & 0xfff)
#define gSPFogPosition(pkt, min, max)				\
	gMoveWd(pkt, G_MW_FOG, G_MWO_FOG, 			\
		(_SHIFTL((128000/((max)-(min))),16,16) |		\
		_SHIFTL(((500-(min))*256/((max)-(min))),0,16)))
#ifdef	F3DEX_GBI_2
# define gSPTexture(pkt, s, t, level, tile, on)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_TEXTURE,24,8) | 			\
			_SHIFTL(BOWTIE_VAL,16,8) |			\
			_SHIFTL((level),11,3) | _SHIFTL((tile),8,3) |	\
			_SHIFTL((on),1,7));				\
	_g->words.w1 = (_SHIFTL((s),16,16) | _SHIFTL((t),0,16));	\
}
# define gsSPTexture(s, t, level, tile, on)				\
{{									\
	(_SHIFTL(G_TEXTURE,24,8) | _SHIFTL(BOWTIE_VAL,16,8) |		\
	 _SHIFTL((level),11,3) | _SHIFTL((tile),8,3) | _SHIFTL((on),1,7)),\
        (_SHIFTL((s),16,16) | _SHIFTL((t),0,16))			\
}}
#else
# define gSPTexture(pkt, s, t, level, tile, on)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_TEXTURE,24,8)|_SHIFTL(BOWTIE_VAL,16,8)|\
			_SHIFTL((level),11,3)|_SHIFTL((tile),8,3)|	\
			_SHIFTL((on),0,8));				\
	_g->words.w1 = (_SHIFTL((s),16,16)|_SHIFTL((t),0,16));		\
}
# define gsSPTexture(s, t, level, tile, on)				\
{{									\
	(_SHIFTL(G_TEXTURE,24,8)|_SHIFTL(BOWTIE_VAL,16,8)|		\
	 _SHIFTL((level),11,3)|_SHIFTL((tile),8,3)|_SHIFTL((on),0,8)),	\
        (_SHIFTL((s),16,16)|_SHIFTL((t),0,16))				\
}}
#endif
#define gSPPerspNormalize(pkt, s)	gMoveWd(pkt, G_MW_PERSPNORM, 0, (s))
#define gsSPPerspNormalize(s)		gsMoveWd(    G_MW_PERSPNORM, 0, (s))
#ifdef	F3DEX_GBI_2
# define gSPPopMatrixN(pkt, n, num)	gDma2p((pkt),G_POPMTX,(num)*64,64,2,0)
# define gsSPPopMatrixN(n, num)		gsDma2p(     G_POPMTX,(num)*64,64,2,0)
# define gSPPopMatrix(pkt, n)		gSPPopMatrixN((pkt), (n), 1)
# define gsSPPopMatrix(n)		gsSPPopMatrixN(      (n), 1)
#else	/* F3DEX_GBI_2 */
# define gSPPopMatrix(pkt, n)		gImmp1(pkt, G_POPMTX, n)
# define gsSPPopMatrix(n)		gsImmp1(    G_POPMTX, n)
#endif	/* F3DEX_GBI_2 */
#define gSPEndDisplayList(pkt)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_ENDDL, 24, 8);				\
	_g->words.w1 = 0;						\
}
#define gsSPEndDisplayList()						\
{{									\
	_SHIFTL(G_ENDDL, 24, 8), 0					\
}}
#ifdef	F3DEX_GBI_2
#define	gSPGeometryMode(pkt, c, s)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = _SHIFTL(G_GEOMETRYMODE,24,8)|_SHIFTL(~(u32)(c),0,24);\
	_g->words.w1 = (u32)(s);					\
}
#define	gsSPGeometryMode(c, s)						\
{{									\
	(_SHIFTL(G_GEOMETRYMODE,24,8)|_SHIFTL(~(u32)(c),0,24)),(u32)(s)	\
}}
#define	gSPSetGeometryMode(pkt, word)	gSPGeometryMode((pkt),0,(word))
#define	gsSPSetGeometryMode(word)	gsSPGeometryMode(0,(word))
#define	gSPClearGeometryMode(pkt, word)	gSPGeometryMode((pkt),(word),0)
#define	gsSPClearGeometryMode(word)	gsSPGeometryMode((word),0)
#define	gSPLoadGeometryMode(pkt, word)	gSPGeometryMode((pkt),-1,(word))
#else	/* F3DEX_GBI_2 */
#define	gSPSetGeometryMode(pkt, word)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_SETGEOMETRYMODE, 24, 8);		\
	_g->words.w1 = (unsigned int)(word);				\
}
#define	gsSPSetGeometryMode(word)					\
{{									\
	_SHIFTL(G_SETGEOMETRYMODE, 24, 8), (unsigned int)(word)		\
}}
#define	gSPClearGeometryMode(pkt, word)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_CLEARGEOMETRYMODE, 24, 8);		\
	_g->words.w1 = (unsigned int)(word);				\
}
#define	gsSPClearGeometryMode(word)					\
{{									\
	_SHIFTL(G_CLEARGEOMETRYMODE, 24, 8), (unsigned int)(word)	\
}}
#endif	/* F3DEX_GBI_2 */
#ifdef	F3DEX_GBI_2
#define	gSPSetOtherMode(pkt, cmd, sft, len, data)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
	_g->words.w0 = (_SHIFTL(cmd,24,8)|_SHIFTL(32-(sft)-(len),8,8)|	\
			_SHIFTL((len)-1,0,8));				\
	_g->words.w1 = (unsigned int)(data);				\
}
#define	gsSPSetOtherMode(cmd, sft, len, data)				\
{{									\
	_SHIFTL(cmd,24,8)|_SHIFTL(32-(sft)-(len),8,8)|_SHIFTL((len)-1,0,8), \
	(unsigned int)(data)						\
}}
#else
#define	gSPSetOtherMode(pkt, cmd, sft, len, data)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) |	\
			_SHIFTL(len, 0, 8));				\
	_g->words.w1 = (unsigned int)(data);				\
}
#define	gsSPSetOtherMode(cmd, sft, len, data)				\
{{									\
	_SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) | _SHIFTL(len, 0, 8),	\
	(unsigned int)(data)						\
}}
#endif
#define	gDPPipelineMode(pkt, mode)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_PIPELINE, 1, mode)
#define	gDPSetCycleType(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#define	gsDPSetCycleType(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#define	gDPSetTexturePersp(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
#define	gsDPSetTexturePersp(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
#define	gDPSetTextureDetail(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTDETAIL, 2, type)
#define	gsDPSetTextureDetail(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTDETAIL, 2, type)
#define	gDPSetTextureLOD(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
#define	gsDPSetTextureLOD(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
#define	gDPSetTextureLUT(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, type)
#define	gsDPSetTextureLUT(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, type)
#define	gDPSetTextureFilter(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
#define	gsDPSetTextureFilter(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
#define	gDPSetTextureConvert(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTCONV, 3, type)
#define	gsDPSetTextureConvert(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_TEXTCONV, 3, type)
#define	gDPSetCombineKey(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_COMBKEY, 1, type)
#define	gsDPSetCombineKey(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_COMBKEY, 1, type)
#ifndef _HW_VERSION_1
#define	gDPSetColorDither(pkt, mode)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2, mode)
#define	gsDPSetColorDither(mode)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2, mode)
#else
#define gDPSetColorDither(pkt, mode)    \
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_COLORDITHER, 1, mode)
#define gsDPSetColorDither(mode)                \
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_COLORDITHER, 1, mode)
#endif
#ifndef _HW_VERSION_1
#define	gDPSetAlphaDither(pkt, mode)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2, mode)
#define	gsDPSetAlphaDither(mode)		\
	gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2, mode)
#endif
#define	gDPSetAlphaCompare(pkt, type)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 2, type)
#define	gsDPSetAlphaCompare(type)		\
	gsSPSetOtherMode(G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 2, type)
#define	gDPSetDepthSource(pkt, src)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_ZSRCSEL, 1, src)
#define	gsDPSetDepthSource(src)	\
	gsSPSetOtherMode(G_SETOTHERMODE_L, G_MDSFT_ZSRCSEL, 1, src)
#define	gDPSetRenderMode(pkt, c0, c1)	\
	gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 	\
			(c0) | (c1))
#define	gsDPSetRenderMode(c0, c1)	\
	gsSPSetOtherMode(G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 	\
			 (c0) | (c1))
#define	gSetImage(pkt, cmd, fmt, siz, width, i)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) |	\
		       _SHIFTL(siz, 19, 2) | _SHIFTL((width)-1, 0, 12);	\
	_g->words.w1 = (unsigned int)(i);				\
}
#define	gDPSetColorImage(pkt, f, s, w, i)	gSetImage(pkt, G_SETCIMG, f, s, w, i)
#define	gsDPSetColorImage(f, s, w, i)		gsSetImage(G_SETCIMG, f, s, w, i)
#define	gDPSetDepthImage(pkt, i)	gSetImage(pkt, G_SETZIMG, 0, 0, 1, i)
#define	gDPSetTextureImage(pkt, f, s, w, i)	gSetImage(pkt, G_SETTIMG, f, s, w, i)
#define	gsDPSetTextureImage(f, s, w, i)	gsSetImage(G_SETTIMG, f, s, w, i)
#define	gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0,	\
		a1, b1, c1, d1,	Aa1, Ab1, Ac1, Ad1)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 =	_SHIFTL(G_SETCOMBINE, 24, 8) |			\
			_SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0,	\
				       G_ACMUX_##Aa0, G_ACMUX_##Ac0) |	\
			       GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1), 	\
			       0, 24);					\
	_g->words.w1 =	(unsigned int)(GCCc0w1(G_CCMUX_##b0, 		\
					       G_CCMUX_##d0,		\
					       G_ACMUX_##Ab0, 		\
					       G_ACMUX_##Ad0) |		\
				       GCCc1w1(G_CCMUX_##b1, 		\
					       G_ACMUX_##Aa1,		\
					       G_ACMUX_##Ac1, 		\
					       G_CCMUX_##d1,		\
					       G_ACMUX_##Ab1, 		\
					       G_ACMUX_##Ad1));		\
}
#define	gsDPSetCombineLERP(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0,		\
		a1, b1, c1, d1,	Aa1, Ab1, Ac1, Ad1)			\
{{									\
	_SHIFTL(G_SETCOMBINE, 24, 8) |					\
	_SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0,			\
		       G_ACMUX_##Aa0, G_ACMUX_##Ac0) |			\
	       GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1), 0, 24),		\
	(unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0,		\
			       G_ACMUX_##Ab0, G_ACMUX_##Ad0) |		\
		       GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1,		\
			       G_ACMUX_##Ac1, G_CCMUX_##d1,		\
			       G_ACMUX_##Ab1, G_ACMUX_##Ad1))		\
}}
#define gDPSetCombineMode(pkt, a, b)	gDPSetCombineLERP(pkt, a, b)
#define	gsDPSetCombineMode(a, b)	gsDPSetCombineLERP(a, b)
#define	gDPSetColor(pkt, c, d)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(c, 24, 8);				\
	_g->words.w1 = (unsigned int)(d);				\
}
#define	gsDPSetColor(c, d)						\
{{									\
	_SHIFTL(c, 24, 8), (unsigned int)(d)				\
}}
#define	DPRGBColor(pkt, cmd, r, g, b, a)				\
            gDPSetColor(pkt, cmd,					\
			(_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | 	\
			 _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
#define	sDPRGBColor(cmd, r, g, b, a)					\
	    gsDPSetColor(cmd,						\
			 (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | 	\
			  _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
#define	gDPSetEnvColor(pkt, r, g, b, a)					\
            DPRGBColor(pkt, G_SETENVCOLOR, r,g,b,a)
#define	gsDPSetEnvColor(r, g, b, a)					\
            sDPRGBColor(G_SETENVCOLOR, r,g,b,a)
#define	gDPSetBlendColor(pkt, r, g, b, a)				\
            DPRGBColor(pkt, G_SETBLENDCOLOR, r,g,b,a)
#define	gDPSetFogColor(pkt, r, g, b, a)					\
            DPRGBColor(pkt, G_SETFOGCOLOR, r,g,b,a)
#define	gDPSetFillColor(pkt, d)						\
            gDPSetColor(pkt, G_SETFILLCOLOR, (d))
#define	gsDPSetFillColor(d)						\
            gsDPSetColor(G_SETFILLCOLOR, (d))
#define	gDPSetPrimDepth(pkt, z, dz)					\
		gDPSetColor(pkt, G_SETPRIMDEPTH, 			\
			    _SHIFTL(z, 16, 16) | _SHIFTL(dz, 0, 16))
#define	gsDPSetPrimDepth(z, dz)						\
		gsDPSetColor(G_SETPRIMDEPTH, _SHIFTL(z, 16, 16) | 	\
			     _SHIFTL(dz, 0, 16))
#define	gDPSetPrimColor(pkt, m, l, r, g, b, a)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 =	(_SHIFTL(G_SETPRIMCOLOR, 24, 8) | 		\
			 _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8));		\
	_g->words.w1 =  (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | 	\
			 _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8));		\
}
#define	gsDPSetPrimColor(m, l, r, g, b, a)				\
{{									\
	(_SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) |		\
	 _SHIFTL(l, 0, 8)),						\
	(_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) |	\
	 _SHIFTL(a, 0, 8))						\
}}
#define	gDPSetOtherMode(pkt, mode0, mode1)				\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_RDPSETOTHERMODE,24,8)|_SHIFTL(mode0,0,24);\
	_g->words.w1 = (unsigned int)(mode1);				\
}
#ifndef MAX
#endif
#ifndef MIN
#endif
#ifdef _HW_VERSION_1
#else
#endif /* _HW_VERSION_1 */
#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt)		\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) |	\
		      _SHIFTL(ult, 0, 12);				\
	_g->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) |	\
		      _SHIFTL(lrt, 0, 12);				\
}
#define gsDPLoadTileGeneric(c, tile, uls, ult, lrs, lrt)		\
{{									\
	_SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),	\
	_SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12)\
}}
#define	gDPSetTileSize(pkt, t, uls, ult, lrs, lrt)			\
		gDPLoadTileGeneric(pkt, G_SETTILESIZE, t, uls, ult, lrs, lrt)
#define	gsDPSetTileSize(t, uls, ult, lrs, lrt)				\
		gsDPLoadTileGeneric(G_SETTILESIZE, t, uls, ult, lrs, lrt)
#define	gDPLoadTile(pkt, t, uls, ult, lrs, lrt)				\
		gDPLoadTileGeneric(pkt, G_LOADTILE, t, uls, ult, lrs, lrt)
#define	gsDPLoadTile(t, uls, ult, lrs, lrt)				\
		gsDPLoadTileGeneric(G_LOADTILE, t, uls, ult, lrs, lrt)
#define	gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt,	\
		maskt, shiftt, cms, masks, shifts)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) |\
		       _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) |	\
		       _SHIFTL(tmem, 0, 9);				\
	_g->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) |	\
		       _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) |	\
		       _SHIFTL(shiftt, 10, 4) |_SHIFTL(cms, 8, 2) |	\
		       _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4);	\
}
#define	gsDPSetTile(fmt, siz, line, tmem, tile, palette, cmt,		\
		maskt, shiftt, cms, masks, shifts)			\
{{									\
	(_SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | 		\
	 _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9)),\
        (_SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | 		\
	 _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) | 			\
	 _SHIFTL(shiftt, 10, 4) | _SHIFTL(cms, 8, 2) | 			\
	 _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4))			\
}}
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_LOADBLOCK, 24, 8) | 			\
			_SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12));	\
	_g->words.w1 = (_SHIFTL(tile, 24, 3) |				\
			_SHIFTL((MIN(lrs,G_TX_LDBLK_MAX_TXL)), 12, 12) |\
			_SHIFTL(dxt, 0, 12));				\
}
#define gsDPLoadBlock(tile, uls, ult, lrs, dxt)				\
{{									\
	(_SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | 		\
	 _SHIFTL(ult, 0, 12)),						\
	(_SHIFTL(tile, 24, 3) | 					\
	 _SHIFTL((MIN(lrs,G_TX_LDBLK_MAX_TXL)), 12, 12) |		\
	 _SHIFTL(dxt, 0, 12))						\
}}
#define	gDPLoadTLUTCmd(pkt, tile, count)				\
{									\
	Gfx *_g = (Gfx *)pkt;						\
									\
	_g->words.w0 = _SHIFTL(G_LOADTLUT, 24, 8);			\
	_g->words.w1 = _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10);\
}
#define	gsDPLoadTLUTCmd(tile, count)					\
{{									\
	_SHIFTL(G_LOADTLUT, 24, 8),					\
	_SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10)		\
}}
#define	gDPLoadTextureBlock(pkt, timg, fmt, siz, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 	\
		0 , cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1,	\
		CALC_DXT(width, siz##_BYTES)); 				\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, 0,			\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	gDPLoadTextureBlockS(pkt, timg, fmt, siz, width, height,	\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 	\
		0 , cmt, maskt,	shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,0);	\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, 0,			\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	_gDPLoadTextureBlock(pkt, timg, tmem, fmt, siz, width, height,	\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, tmem, G_TX_LOADTILE,  \
		0, cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,	\
		CALC_DXT(width, siz##_BYTES)); 				\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3,	\
		tmem, G_TX_RENDERTILE, pal, cmt,			\
		maskt, shiftt, cms, masks, shifts);			\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	_gDPLoadTextureBlockTile(pkt, timg, tmem, rtile, fmt, siz, width,  \
		height, pal, cms, cmt, masks, maskt, shifts, shiftt)	\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, tmem, G_TX_LOADTILE, 0,\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,	\
		CALC_DXT(width, siz##_BYTES)); 				\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3,	\
		tmem, rtile, pal, cmt,					\
		maskt, shiftt, cms, masks, shifts);			\
	gDPSetTileSize(pkt, rtile, 0, 0,				\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	gsDPLoadTextureBlock(timg, fmt, siz, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
									\
	gsDPSetTextureImage(fmt, siz##_LOAD_BLOCK, 1, timg),		\
	gsDPSetTile(fmt, siz##_LOAD_BLOCK, 0, 0, 			\
		G_TX_LOADTILE, 	0 , cmt, maskt,	shiftt, cms, 		\
		masks, shifts),						\
	gsDPLoadSync(),							\
	gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,	\
		CALC_DXT(width, siz##_BYTES)), 				\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, siz, ((((width) * siz##_LINE_BYTES)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE, 0, 0,				\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)
#define	_gsDPLoadTextureBlock(timg, tmem, fmt, siz, width, height,	\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
									\
	gsDPSetTextureImage(fmt, siz##_LOAD_BLOCK, 1, timg),		\
	gsDPSetTile(fmt, siz##_LOAD_BLOCK, 0, tmem, G_TX_LOADTILE, 	\
		0 , cmt, maskt,	shiftt, cms, masks, shifts),		\
	gsDPLoadSync(),							\
	gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,	\
		CALC_DXT(width, siz##_BYTES)), 				\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, siz,						\
		((((width) * siz##_LINE_BYTES)+7)>>3), tmem,		\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE, 0, 0,				\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)
#define	_gsDPLoadTextureBlockTile(timg, tmem, rtile, fmt, siz, width,	\
		height, pal, cms, cmt, masks, maskt, shifts, shiftt)	\
									\
	gsDPSetTextureImage(fmt, siz##_LOAD_BLOCK, 1, timg),		\
	gsDPSetTile(fmt, siz##_LOAD_BLOCK, 0, tmem, G_TX_LOADTILE, 	\
		0 , cmt, maskt,	shiftt, cms, masks, shifts),		\
	gsDPLoadSync(),							\
	gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT)-1,	\
		CALC_DXT(width, siz##_BYTES)), 				\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, siz,						\
		((((width) * siz##_LINE_BYTES)+7)>>3), tmem,		\
		rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		shifts),						\
	gsDPSetTileSize(rtile, 0, 0,					\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)
#define	gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width)); 					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	_gDPLoadTextureBlock_4b(pkt, timg, tmem, fmt, width, height,	\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, tmem, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width));					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), tmem,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	gsDPLoadTextureBlock_4b(timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
									\
	gsDPSetTextureImage(fmt, G_IM_SIZ_16b, 1, timg),		\
	gsDPSetTile(fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0 , cmt,	\
		maskt, shiftt, cms, masks, shifts),			\
	gsDPLoadSync(),							\
	gsDPLoadBlock(G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1,	\
		CALC_DXT_4b(width)),					\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,		\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE, 0, 0,				\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)
#define	_gsDPLoadTextureBlock_4b(timg, tmem, fmt, width, height,	\
		pal, cms, cmt, masks, maskt, shifts, shiftt)		\
									\
	gsDPSetTextureImage(fmt, G_IM_SIZ_16b, 1, timg),		\
	gsDPSetTile(fmt, G_IM_SIZ_16b, 0, tmem, G_TX_LOADTILE, 0 , cmt,	\
		maskt, shiftt, cms, masks, shifts),			\
	gsDPLoadSync(),							\
	gsDPLoadBlock(G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1,	\
		CALC_DXT_4b(width)), 					\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), tmem,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE, 0, 0,				\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1) << G_TEXTURE_IMAGE_FRAC)
#ifndef _HW_VERSION_1
#define	gDPLoadTextureTile(pkt, timg, fmt, siz, width, height,		\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
{									\
	gDPSetTextureImage(pkt, fmt, siz, width, timg);			\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0,	\
		G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC);			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)			\
}
#else /******** WORKAROUND hw 1 load tile bug ********/
#define	gDPLoadTextureTile(pkt, timg, fmt, siz, width, height,		\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
									\
{									\
	int _loadtile_i, _loadtile_nw; Gfx *_loadtile_temp = pkt; 	\
	guDPLoadTextureTile(_loadtile_temp, timg, fmt, siz, 		\
		width, height,        					\
                uls, ult, lrs, lrt, pal,                                \
                cms, cmt, masks, maskt, shifts, shiftt);		\
	_loadtile_nw = guGetDPLoadTextureTileSz(ult, lrt) - 1;		\
	for(_loadtile_i = 0; _loadtile_i < _loadtile_nw; _loadtile_i++)	\
	  pkt;								\
}
#endif /* HW_VERSION_1 */
#define	gDPLoadMultiTile(pkt, timg, tmem, rtile, fmt, siz, width, height,\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
{									\
	gDPSetTextureImage(pkt, fmt, siz, width, timg);			\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem,	\
		G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC);			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem,	\
		rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)			\
}
#define	gsDPLoadTextureTile(timg, fmt, siz, width, height,		\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
									\
	gsDPSetTextureImage(fmt, siz, width, timg),			\
	gsDPSetTile(fmt, siz, 						\
		    (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0,	\
		    G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		    shifts),						\
	gsDPLoadSync(),							\
	gsDPLoadTile(	G_TX_LOADTILE,					\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC),			\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, siz, 						\
		    (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0,	\
		    G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,\
		    shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)
#define	gsDPLoadMultiTile(timg, tmem, rtile, fmt, siz, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
									\
	gsDPSetTextureImage(fmt, siz, width, timg),			\
	gsDPSetTile(fmt, siz, 						\
		    (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 	\
		    tmem, G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, 	\
		    masks, shifts),					\
	gsDPLoadSync(),							\
	gsDPLoadTile(	G_TX_LOADTILE,					\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC),			\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, siz, 						\
		    (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 	\
		    tmem, rtile, pal, cmt, maskt, shiftt, cms, masks,	\
		    shifts),						\
	gsDPSetTileSize(rtile,						\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)
#define	gDPLoadTextureTile_4b(pkt, timg, fmt, width, height,		\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, 	\
		   masks, shifts);					\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)			\
}
#define	gDPLoadMultiTile_4b(pkt, timg, tmem, rtile, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)			\
}
#define	gsDPLoadTextureTile_4b(timg, fmt, width, height,		\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
									\
	gsDPSetTextureImage(fmt, G_IM_SIZ_8b, ((width)>>1), timg),	\
	gsDPSetTile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0,	\
		G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPLoadSync(),							\
	gsDPLoadTile(	G_TX_LOADTILE,					\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC)),			\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts),						\
	gsDPSetTileSize(G_TX_RENDERTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)
#define	gsDPLoadMultiTile_4b(timg, tmem, rtile, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt)			\
									\
	gsDPSetTextureImage(fmt, G_IM_SIZ_8b, ((width)>>1), timg),	\
	gsDPSetTile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 	\
		    tmem, G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, 	\
		    masks, shifts),					\
	gsDPLoadSync(),							\
	gsDPLoadTile(	G_TX_LOADTILE,					\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC)),			\
	gsDPPipeSync(),							\
	gsDPSetTile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 	\
		    tmem, rtile, pal, cmt, maskt, shiftt, cms, masks,	\
		    shifts),						\
	gsDPSetTileSize(rtile,						\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC)
#ifndef _HW_VERSION_1
#define gDPLoadTLUT_pal16(pkt, pal, dram)				\
{									\
	gDPSetTextureImage(pkt, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram);	\
	gDPTileSync(pkt);						\
	gDPSetTile(pkt, 0, 0, 0, (256+(((pal)&0xf)*16)),		\
		G_TX_LOADTILE, 0 , 0, 0, 0, 0, 0, 0);			\
	gDPLoadSync(pkt);						\
	gDPLoadTLUTCmd(pkt, G_TX_LOADTILE, 15);				\
	gDPPipeSync(pkt)						\
}
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#define gDPLoadTLUT_pal16(pkt, pal, dram)				\
									\
        _gDPLoadTextureBlock(pkt, dram, (256+(((pal)&0xf)*16)),		\
		G_IM_FMT_RGBA, G_IM_SIZ_16b, 4*16, 1,			\
		pal, 0, 0, 0, 0, 0, 0)
#endif /* _HW_VERSION_1 */
#ifndef _HW_VERSION_1
#define gsDPLoadTLUT_pal16(pal, dram)					\
 									\
	gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram),	\
	gsDPTileSync(),							\
	gsDPSetTile(0, 0, 0, (256+(((pal)&0xf)*16)),			\
		G_TX_LOADTILE, 0 , 0, 0, 0, 0, 0, 0),			\
	gsDPLoadSync(),							\
	gsDPLoadTLUTCmd(G_TX_LOADTILE, 15),				\
	gsDPPipeSync()
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#define gsDPLoadTLUT_pal16(pal, dram)                                   \
									\
	_gsDPLoadTextureBlock(dram, (256+(((pal)&0xf)*16)),           	\
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 4*16, 1,                   \
                pal, 0, 0, 0, 0, 0, 0)
#endif /* _HW_VERSION_1 */
#ifndef _HW_VERSION_1
#define gDPLoadTLUT_pal256(pkt, dram)					\
{									\
	gDPSetTextureImage(pkt, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram);	\
	gDPTileSync(pkt);						\
	gDPSetTile(pkt, 0, 0, 0, 256,					\
		G_TX_LOADTILE, 0 , 0, 0, 0, 0, 0, 0);			\
	gDPLoadSync(pkt);						\
	gDPLoadTLUTCmd(pkt, G_TX_LOADTILE, 255);			\
	gDPPipeSync(pkt)						\
}
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#define gDPLoadTLUT_pal256(pkt, dram)                                   \
	                                                                \
	_gDPLoadTextureBlock(pkt, dram, 256,           			\
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 4*256, 1,			\
                0, 0, 0, 0, 0, 0, 0)
#endif /* _HW_VERSION_1 */
#ifndef _HW_VERSION_1
#define gsDPLoadTLUT_pal256(dram)					\
 									\
	gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram),	\
	gsDPTileSync(),							\
	gsDPSetTile(0, 0, 0, 256,					\
		G_TX_LOADTILE, 0 , 0, 0, 0, 0, 0, 0),			\
	gsDPLoadSync(),							\
	gsDPLoadTLUTCmd(G_TX_LOADTILE, 255),				\
	gsDPPipeSync()
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#define gsDPLoadTLUT_pal256(dram)                                       \
                                                                        \
	_gsDPLoadTextureBlock(dram, 256,                            	\
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 4*256, 1,                  \
                0, 0, 0, 0, 0, 0, 0)
#endif /* _HW_VERSION_1 */
#ifndef _HW_VERSION_1
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#endif /* _HW_VERSION_1 */
#ifndef _HW_VERSION_1
#else /* **** WORKAROUND hardware 1 load_tlut bug ****** */
#endif /* _HW_VERSION_1 */
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)			\
{									\
	Gfx *_g = (Gfx *)pkt;						\
									\
	_g->words.w0 = _SHIFTL(G_SETSCISSOR, 24, 8) |			\
		       _SHIFTL((int)((float)(ulx)*4.0F), 12, 12) |	\
                       _SHIFTL((int)((float)(uly)*4.0F), 0, 12);	\
	_g->words.w1 = _SHIFTL(mode, 24, 2) |				\
		       _SHIFTL((int)((float)(lrx)*4.0F), 12, 12) |	\
                       _SHIFTL((int)((float)(lry)*4.0F), 0, 12);	\
}
#define gDPSetScissorFrac(pkt, mode, ulx, uly, lrx, lry)		\
{									\
	Gfx *_g = (Gfx *)pkt;						\
									\
	_g->words.w0 = _SHIFTL(G_SETSCISSOR, 24, 8) |			\
		       _SHIFTL((int)((ulx)), 12, 12) |			\
                       _SHIFTL((int)((uly)), 0, 12);			\
	_g->words.w1 = _SHIFTL(mode, 24, 2) |				\
		       _SHIFTL((int)((lrx)), 12, 12) | 			\
                       _SHIFTL((int)((lry)), 0, 12);			\
}
#define gsDPSetScissor(mode, ulx, uly, lrx, lry)			\
{{									\
	_SHIFTL(G_SETSCISSOR, 24, 8) |					\
	_SHIFTL((int)((float)(ulx)*4.0F), 12, 12) |			\
	_SHIFTL((int)((float)(uly)*4.0F), 0, 12),			\
	_SHIFTL(mode, 24, 2) |						\
	_SHIFTL((int)((float)(lrx)*4.0F), 12, 12) |			\
	_SHIFTL((int)((float)(lry)*4.0F), 0, 12)			\
}}
#define gsDPSetScissorFrac(mode, ulx, uly, lrx, lry)			\
{{									\
	_SHIFTL(G_SETSCISSOR, 24, 8) |					\
	_SHIFTL((int)((ulx)), 12, 12) |					\
	_SHIFTL((int)((uly)), 0, 12),					\
	_SHIFTL(mode, 24, 2) |						\
	_SHIFTL((int)(lrx), 12, 12) |					\
	_SHIFTL((int)(lry), 0, 12)					\
}}
#define	gDPFillRectangle(pkt, ulx, uly, lrx, lry)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_FILLRECT, 24, 8) | 			\
			_SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10));\
	_g->words.w1 = (_SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10));\
}
#define	gsDPFillRectangle(ulx, uly, lrx, lry)				\
{{									\
	(_SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | 		\
	 _SHIFTL((lry), 2, 10)),					\
	(_SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10))		\
}}
#define	gDPScisFillRectangle(pkt, ulx, uly, lrx, lry)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_FILLRECT, 24, 8) | 			\
			_SHIFTL(MAX((lrx),0), 14, 10) | 		\
			_SHIFTL(MAX((lry),0), 2, 10));			\
	_g->words.w1 = (_SHIFTL(MAX((ulx),0), 14, 10) | 		\
			_SHIFTL(MAX((uly),0), 2, 10));			\
}
#define	gDPSetConvert(pkt, k0, k1, k2, k3, k4, k5)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_SETCONVERT, 24, 8) |			\
			_SHIFTL(k0, 13, 9) | _SHIFTL(k1, 4, 9) | 	\
			_SHIFTR(k2, 5, 4));				\
	_g->words.w1 = (_SHIFTL(k2, 27, 5) | _SHIFTL(k3, 18, 9) |	\
			_SHIFTL(k4, 9, 9) | _SHIFTL(k5, 0, 9));		\
}
#define gsDPSetConvert(k0, k1, k2, k3, k4, k5)				\
{{									\
	(_SHIFTL(G_SETCONVERT, 24, 8) |					\
	 _SHIFTL(k0, 13, 9) | _SHIFTL(k1, 4, 9) | _SHIFTR(k2, 5, 4)),	\
	(_SHIFTL(k2, 27, 5) | _SHIFTL(k3, 18, 9) | _SHIFTL(k4, 9, 9) | 	\
	 _SHIFTL(k5, 0, 9))						\
}}
#define	gDPSetKeyR(pkt, cR, sR, wR)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(G_SETKEYR, 24, 8);			\
	_g->words.w1 = (_SHIFTL(wR, 16, 12) | _SHIFTL(cR, 8, 8) | 	\
			_SHIFTL(sR, 0, 8));				\
}
#define	gsDPSetKeyR(cR, sR, wR)						\
{{									\
	_SHIFTL(G_SETKEYR, 24, 8),					\
	_SHIFTL(wR, 16, 12) | _SHIFTL(cR, 8, 8) | _SHIFTL(sR, 0, 8)	\
}}
#define	gDPSetKeyGB(pkt, cG, sG, wG, cB, sB, wB)			\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = (_SHIFTL(G_SETKEYGB, 24, 8) |			\
			_SHIFTL(wG, 12, 12) | _SHIFTL(wB, 0, 12));	\
	_g->words.w1 = (_SHIFTL(cG, 24, 8) | _SHIFTL(sG, 16, 8) |	\
			 _SHIFTL(cB, 8, 8) | _SHIFTL(sB, 0, 8));	\
}
#define	gsDPSetKeyGB(cG, sG, wG, cB, sB, wB)				\
{{									\
	(_SHIFTL(G_SETKEYGB, 24, 8) | _SHIFTL(wG, 12, 12) |		\
	 _SHIFTL(wB, 0, 12)),						\
	(_SHIFTL(cG, 24, 8) | _SHIFTL(sG, 16, 8) | _SHIFTL(cB, 8, 8) |	\
	 _SHIFTL(sB, 0, 8))						\
}}
#define gDPNoParam(pkt, cmd)						\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(cmd, 24, 8);				\
	_g->words.w1 = 0;						\
}
#define gsDPNoParam(cmd)						\
{{									\
	_SHIFTL(cmd, 24, 8), 0						\
}}
#define gDPParam(pkt, cmd, param)					\
{									\
	Gfx *_g = (Gfx *)(pkt);						\
									\
	_g->words.w0 = _SHIFTL(cmd, 24, 8);				\
	_g->words.w1 = (param);						\
}
#define gsDPParam(cmd, param)						\
{{									\
	_SHIFTL(cmd, 24, 8), (param)					\
}}
#define gsDPTextureRectangle(xl, yl, xh, yh, tile, s, t, dsdx, dtdy)	\
{{									\
    (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | 			\
     _SHIFTL(yh, 0, 12)),						\
    (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12)),	\
}},									\
{{									\
    _SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16),				\
    _SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)			\
}}
#define gDPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)\
{									\
    Gfx *_g = (Gfx *)(pkt);						\
    if (pkt);								\
    _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) |	\
		    _SHIFTL(yh, 0, 12));    				\
    _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) |	\
		    _SHIFTL(yl, 0, 12));				\
    _g ++;								\
    _g->words.w0 = (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16));		\
    _g->words.w1 = (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16));	\
}
#define gsSPTextureRectangle(xl, yl, xh, yh, tile, s, t, dsdx, dtdy)	\
    {{(_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12)),\
    (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12))}},	\
    gsImmp1(G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))),	\
    gsImmp1(G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)))
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)\
{									\
    Gfx *_g = (Gfx *)(pkt);						\
									\
    _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) |	\
		    _SHIFTL(yh, 0, 12));    				\
    _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) |	\
		    _SHIFTL(yl, 0, 12));				\
    gImmp1(pkt, G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)));	\
    gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)));\
}
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
{                                                                            \
    Gfx *_g = (Gfx *)(pkt);                                                  \
                                                                             \
    _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) |                              \
                    _SHIFTL(PM_SCIS_XH(xh), 12, 12) |                        \
                    _SHIFTL(MAX((s16)(yh),0), 0, 12));                       \
    _g->words.w1 = (_SHIFTL((tile), 24, 3) |                                 \
                    _SHIFTL(PM_SCIS_XL(xl), 12, 12) |                      \
                    _SHIFTL(MAX((s16)(yl),0), 0, 12));                       \
    gImmp1(pkt, G_RDPHALF_1,                                                 \
                (_SHIFTL(((s) -                                              \
                          (PM_SCIS_CLIP(xl) ?                                 \
                           (((s16)(dsdx) < 0) ?                              \
                            (MAX((((s16)(xl)*(s16)(dsdx))>>7),0)) :          \
			    (MIN((((s16)(xl)*(s16)(dsdx))>>7),0))) : 0)),    \
			 16, 16) |                                           \
                 _SHIFTL(((t) -                                              \
                          (((yl) < 0) ?                                      \
                           (((s16)(dtdy) < 0) ?                              \
                            (MAX((((s16)(yl)*(s16)(dtdy))>>7),0)) :          \
                            (MIN((((s16)(yl)*(s16)(dtdy))>>7),0))) : 0)),    \
			 0, 16)));                                           \
    gImmp1(pkt, G_RDPHALF_2, (_SHIFTL((dsdx), 16, 16) |                      \
                              _SHIFTL((dtdy), 0, 16)));                      \
}
#define gsSPTextureRectangleFlip(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    {{(_SHIFTL(G_TEXRECTFLIP, 24, 8) | _SHIFTL(xh, 12, 12) |		\
     _SHIFTL(yh, 0, 12)),						\
    (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12))}},	\
    gsImmp1(G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))),	\
    gsImmp1(G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)))
#define gSPTextureRectangleFlip(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
{								       	\
    Gfx *_g = (Gfx *)(pkt);					       	\
									 \
    _g->words.w0 = (_SHIFTL(G_TEXRECTFLIP, 24, 8) | _SHIFTL(xh, 12, 12) |\
		    _SHIFTL(yh, 0, 12)); 				\
    _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) |	\
		    _SHIFTL(yl, 0, 12));				\
    gImmp1(pkt, G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)));	\
    gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))); \
}
#define	gDPFullSync(pkt)	gDPNoParam(pkt, G_RDPFULLSYNC)
#define	gDPTileSync(pkt)	gDPNoParam(pkt, G_RDPTILESYNC)
#define	gsDPTileSync()		gsDPNoParam(G_RDPTILESYNC)
#define	gDPPipeSync(pkt)	gDPNoParam(pkt, G_RDPPIPESYNC)
#define	gsDPPipeSync()		gsDPNoParam(G_RDPPIPESYNC)
#define	gDPLoadSync(pkt)	gDPNoParam(pkt, G_RDPLOADSYNC)
#define	gsDPLoadSync()		gsDPNoParam(G_RDPLOADSYNC)
#define	gDPNoOp(pkt)		gDPNoParam(pkt, G_NOOP)
#define	gsDPNoOp()		gsDPNoParam(G_NOOP)
#endif /* _LANGUAGE_C */
#define	gDPScrollMultiTile2_4b(pkt, timg, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt,\
    scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE + 1, 0, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,					\
			(uls + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult + scrollt)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt + scrollt)<<G_TEXTURE_IMAGE_FRAC)			\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE + 0, 1, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
}
#define	gDPScrollTextureTile_4b(pkt, timg, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt, \
    scrolls, scrollt)			\
							\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), 0,			\
		   G_TX_RENDERTILE, 0, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,					\
			(uls + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult + scrollt)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs + scrolls)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt + scrollt)<<G_TEXTURE_IMAGE_FRAC)			\

#define	gDPScrollTextureBlock_4b(pkt, timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width)); 					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE,\
        (scrolls) << G_TEXTURE_IMAGE_FRAC,\
        (scrollt) << G_TEXTURE_IMAGE_FRAC,			\
		((width)-1 + scrolls) << G_TEXTURE_IMAGE_FRAC,			\
		((height)-1 + scrollt) << G_TEXTURE_IMAGE_FRAC)			\
}
#define	gDPScrollMultiTile_4b(pkt, timg, tmem, rtile, fmt, width, height,	\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt,\
    scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width)>>1), timg);	\
	gDPSetTile(pkt, fmt, G_IM_SIZ_8b, 			        \
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		   shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(ult)<<(G_TEXTURE_IMAGE_FRAC),			\
			(lrs)<<(G_TEXTURE_IMAGE_FRAC-1),		\
			(lrt)<<(G_TEXTURE_IMAGE_FRAC));			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, 				\
		   (((((lrs)-(uls)+1)>>1)+7)>>3), tmem,			\
		   rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		   shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt,			\
			((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt);      \
}
#define	gDPScrollMultiTile(pkt, timg, tmem, rtile, fmt, siz, width, height,\
		uls, ult, lrs, lrt, pal,				\
		cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt)			\
{									\
	gDPSetTextureImage(pkt, fmt, siz, width, timg);			\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem,	\
		G_TX_LOADTILE, 0 , cmt, maskt, shiftt, cms, masks,	\
		shifts);						\
	gDPLoadSync(pkt);						\
	gDPLoadTile(	pkt, G_TX_LOADTILE,				\
			(uls)<<G_TEXTURE_IMAGE_FRAC,			\
			(ult)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrs)<<G_TEXTURE_IMAGE_FRAC,			\
			(lrt)<<G_TEXTURE_IMAGE_FRAC);			\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem,	\
		rtile, pal, cmt, maskt, shiftt, cms, masks,		\
		shifts);						\
	gDPSetTileSize(pkt, rtile,					\
			((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt,			\
			((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls,			\
			((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt);      \
}
#define	gDPScrollTextureBlockHalfHeight(pkt, timg, fmt, siz, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2)		\
{									\
	gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);	\
	gDPSetTile(pkt, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 	\
		0 , cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0, 				\
		(((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1,	\
		CALC_DXT(width, siz##_BYTES)); 				\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, 0,			\
		G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		(((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)			\
    gDPSetTile(pkt, fmt, siz,					\
		(((width) * siz##_LINE_BYTES)+7)>>3, \
        (((width) * ((height) >> 1) * siz##_LINE_BYTES)+7)>>3,			\
		G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks,	\
		shifts2);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,			\
        (scrolls),\
        (scrollt),			\
		(((width)-1) << G_TEXTURE_IMAGE_FRAC) + (scrolls),			\
		((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)  + (scrollt)	);		\
}
#define	gDPScrollTextureBlockHalfHeight_4b(pkt, timg, fmt, width, height,		\
		pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2)		\
{									\
	gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);		\
	gDPSetTile(pkt, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,	\
		cmt, maskt, shiftt, cms, masks, shifts);		\
	gDPLoadSync(pkt);						\
	gDPLoadBlock(pkt, G_TX_LOADTILE, 0, 0,				\
		(((width)*(height)+3)>>2)-1,				\
		CALC_DXT_4b(width)); 					\
	gDPPipeSync(pkt);						\
	gDPSetTile(pkt, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0,	\
		G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks,	\
		shifts);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE, 0, 0,			\
		((width)-1) << G_TEXTURE_IMAGE_FRAC,			\
		(((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)			\
    gDPSetTile(pkt, fmt, G_IM_SIZ_4b,					\
		(((width)>>1)+7)>>3, \
        (((width) * ((height) >> 1) / 2)+7)>>3,			\
		G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks,	\
		shifts2);						\
	gDPSetTileSize(pkt, G_TX_RENDERTILE + 1,			\
        scrolls,\
        scrollt,			\
		(((width)-1) << G_TEXTURE_IMAGE_FRAC) + scrolls,			\
		((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC)  + scrollt)		\
}
#define gPmNdSplash(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_SPLASH << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }
#define gPmNdGutter(pkt, a) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GUTTER << 16) | ((u32)(a) & 0xffu); _g->words.w1 = 0; }
#define gPmNdGrabHold(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GRABHOLD << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }
#define gPmNdSprShade(pkt, on) { Gfx *_g = (Gfx *)(pkt); _g->words.w0 = ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_SPRSHADE << 16) | ((on) ? 1u : 0u); _g->words.w1 = 0; }
#endif
