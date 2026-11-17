#include "mac_00.h"
#include "model.h"
#include "nd_val.h"
#include "nd_st.h"
#include "nd_geo.h"
#include "nd_at.h"

MAP_RODATA_PAD(1,unk);

void N(gfx_build_water)(void) {
    Model* model;

    nd_pso_zsrc(G_ZS_PRIM);
    nd_val_primz(0x7FF0, 0);
    nd_pso_rm(Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | G_RM_PASS, Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    nd_pso_cc(PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA);
    nd_val_prim(0, 0, 0, 0, 128, 0);

    model = get_model_from_list_index(get_model_list_index_from_tree_index(MODEL_water));

    NDCTL_CALL(nd_nr, model->modelNode->displayData->displayList); /* NMDL7: the list call unit (nd_at.h:35) */
    nd_pso_cyc(G_CYC_2CYCLE);
    nd_pso_zsrc(G_ZS_PIXEL);
    nd_pso_rm(G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);

    mdl_draw_hidden_panel_surface(MODEL_water);
}

EvtScript N(EVS_SetupWaterCustomGfx) = {
    Wait(1)
    Call(SetCustomGfxBuilders, CUSTOM_GFX_0, Ref(N(gfx_build_water)), nullptr)
    Call(SetModelCustomGfx, MODEL_water, CUSTOM_GFX_0, -1)
    Return
    End
};
