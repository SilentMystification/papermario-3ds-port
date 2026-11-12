/* nd_rmdef.h: the render-mode lists of src/model.c (Gfx_RM*) as S6T defs (nd_scomp.h). ND_RM_REC(Gfx_RMk_NAME) records the
 * compiled range of that list instead of a gSPDisplayList (S5EMDL1). */
#ifndef ND_RMDEF_H
#define ND_RMDEF_H
#include "nd_scomp.h"
#define ND_RM_LIST(X) \
    X(Gfx_RM1_SURFACE_OPA) \
    X(Gfx_RM1_DECAL_OPA) \
    X(Gfx_RM1_INTERSECTING_OPA) \
    X(Gfx_RM1_ALPHATEST) \
    X(Gfx_RM1_SURFACE_XLU) \
    X(Gfx_RM1_DECAL_XLU) \
    X(Gfx_RM1_INTERSECTING_XLU) \
    X(Gfx_RM1_SURFACE_OPA_NO_AA) \
    X(Gfx_RM1_DECAL_OPA_NO_AA) \
    X(Gfx_RM1_INTERSECTING_OPA_NO_AA) \
    X(Gfx_RM1_ALPHATEST_ONESIDED) \
    X(Gfx_RM1_SURFACE_XLU_NO_AA) \
    X(Gfx_RM1_DECAL_XLU_NO_AA) \
    X(Gfx_RM1_PASS_THROUGH) \
    X(Gfx_RM1_SURFACE_XLU_AA_ZB_ZUPD) \
    X(Gfx_RM1_SURFACE_OPA_NO_ZB) \
    X(Gfx_RM1_ALPHATEST_NO_ZB) \
    X(Gfx_RM1_SURFACE_XLU_NO_ZB) \
    X(Gfx_RM1_SURFACE_XLU_ZB_ZUPD) \
    X(Gfx_RM1_CLOUD_NO_ZCMP) \
    X(Gfx_RM1_CLOUD) \
    X(Gfx_RM1_CLOUD_NO_ZB) \
    X(Gfx_RM2_SURFACE_OPA) \
    X(Gfx_RM2_DECAL_OPA) \
    X(Gfx_RM2_INTERSECTING_OPA) \
    X(Gfx_RM2_ALPHATEST) \
    X(Gfx_RM2_SURFACE_XLU) \
    X(Gfx_RM2_DECAL_XLU) \
    X(Gfx_RM2_INTERSECTING_XLU) \
    X(Gfx_RM2_SURFACE_OPA_NO_AA) \
    X(Gfx_RM2_DECAL_OPA_NO_AA) \
    X(Gfx_RM2_INTERSECTING_OPA_NO_AA) \
    X(Gfx_RM2_ALPHATEST_ONESIDED) \
    X(Gfx_RM2_SURFACE_XLU_NO_AA) \
    X(Gfx_RM2_DECAL_XLU_NO_AA) \
    X(Gfx_RM2_PASS_THROUGH) \
    X(Gfx_RM2_SURFACE_XLU_AA_ZB_ZUPD) \
    X(Gfx_RM2_SURFACE_OPA_NO_ZB) \
    X(Gfx_RM2_ALPHATEST_NO_ZB) \
    X(Gfx_RM2_SURFACE_XLU_NO_ZB) \
    X(Gfx_RM2_CLOUD) \
    X(Gfx_RM2_CLOUD_NO_ZB) \
    X(Gfx_RM3_SURFACE_OPA) \
    X(Gfx_RM3_DECAL_OPA) \
    X(Gfx_RM3_INTERSECTING_OPA) \
    X(Gfx_RM3_ALPHATEST) \
    X(Gfx_RM3_SURFACE_XLU) \
    X(Gfx_RM3_DECAL_XLU) \
    X(Gfx_RM3_INTERSECTING_XLU) \
    X(Gfx_RM3_SURFACE_OPA_NO_AA) \
    X(Gfx_RM3_DECAL_OPA_NO_AA) \
    X(Gfx_RM3_INTERSECTING_OPA_NO_AA) \
    X(Gfx_RM3_ALPHATEST_ONESIDED) \
    X(Gfx_RM3_SURFACE_XLU_NO_AA) \
    X(Gfx_RM3_DECAL_XLU_NO_AA) \
    X(Gfx_RM3_PASS_THROUGH) \
    X(Gfx_RM3_SURFACE_XLU_AA_ZB_ZUPD) \
    X(Gfx_RM3_SURFACE_OPA_NO_ZB) \
    X(Gfx_RM3_ALPHATEST_NO_ZB) \
    X(Gfx_RM3_SURFACE_XLU_NO_ZB) \
    X(Gfx_RM3_CLOUD) \
    X(Gfx_RM3_CLOUD_NO_ZB)
#define ND_RM_EXT(n) extern NdSDef n##_def;
ND_RM_LIST(ND_RM_EXT)
#define ND_RM_REC(n) nd_scomp_rec(&n##_def, NULL, 0)
#endif
