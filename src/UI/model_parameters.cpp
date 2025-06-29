#include "UI/model_parameters.h"
#include "imgui.h"

// --- Tableaux de descripteurs (AddLiq/PriceDist) ---
static DoubleParamDesc addLiqPriceDistParams[] = {
    { "mu_jitter", &gSimuParams.addLiq.priceDist.mu_jitter, &gSimuParamBounds.addLiq.priceDist.mu_jitter },
    { "mu_init", &gSimuParams.addLiq.priceDist.mu_init, &gSimuParamBounds.addLiq.priceDist.mu_init },
    { "p_birth", &gSimuParams.addLiq.priceDist.p_birth, &gSimuParamBounds.addLiq.priceDist.p_birth },
    { "p_death", &gSimuParams.addLiq.priceDist.p_death, &gSimuParamBounds.addLiq.priceDist.p_death },
    { "sigma_jitter", &gSimuParams.addLiq.priceDist.sigma_jitter, &gSimuParamBounds.addLiq.priceDist.sigma_jitter },
    { "sigma_init", &gSimuParams.addLiq.priceDist.sigma_init, &gSimuParamBounds.addLiq.priceDist.sigma_init },
    { "amplitudeBrownian", &gSimuParams.addLiq.priceDist.amplitudeBrownian, &gSimuParamBounds.addLiq.priceDist.amplitudeBrownian },
    { "powDistParam", &gSimuParams.addLiq.priceDist.powDistParam, &gSimuParamBounds.addLiq.priceDist.powDistParam },
    { "weight_brownian", &gSimuParams.addLiq.priceDist.weight_brownian, &gSimuParamBounds.addLiq.priceDist.weight_brownian },
    { "weight_power", &gSimuParams.addLiq.priceDist.weight_power, &gSimuParamBounds.addLiq.priceDist.weight_power },
};
static constexpr int NB_ADDLIQ_PARAMS = sizeof(addLiqPriceDistParams) / sizeof(DoubleParamDesc);

// --- Tableaux de descripteurs (RemoveLiq) ---
static DoubleParamDesc removeLiqParams[] = {
    { "mu_jitter##remove", &gSimuParams.removeLiq.mu_jitter, &gSimuParamBounds.removeLiq.mu_jitter },
    { "mu_init##remove", &gSimuParams.removeLiq.mu_init, &gSimuParamBounds.removeLiq.mu_init },
    { "p_birth##remove", &gSimuParams.removeLiq.p_birth, &gSimuParamBounds.removeLiq.p_birth },
    { "p_death##remove", &gSimuParams.removeLiq.p_death, &gSimuParamBounds.removeLiq.p_death },
    { "sigma_jitter##remove", &gSimuParams.removeLiq.sigma_jitter, &gSimuParamBounds.removeLiq.sigma_jitter },
    { "sigma_init##remove", &gSimuParams.removeLiq.sigma_init, &gSimuParamBounds.removeLiq.sigma_init },
    { "amplitudeBrownian##remove", &gSimuParams.removeLiq.amplitudeBrownian, &gSimuParamBounds.removeLiq.amplitudeBrownian },
    { "powDistParam##remove", &gSimuParams.removeLiq.powDistParam, &gSimuParamBounds.removeLiq.powDistParam },
    { "weight_brownian##remove", &gSimuParams.removeLiq.weight_brownian, &gSimuParamBounds.removeLiq.weight_brownian },
    { "weight_power##remove", &gSimuParams.removeLiq.weight_power, &gSimuParamBounds.removeLiq.weight_power },
};
static constexpr int NB_REMOVELIQ_PARAMS = sizeof(removeLiqParams) / sizeof(DoubleParamDesc);

// --- Tableaux de descripteurs (MarketOrder) ---
static DoubleParamDesc marketOrderParams[] = {
    { "e", &gSimuParams.marketOrder.e, &gSimuParamBounds.marketOrder.e }
};
static constexpr int NB_MARKETORDER_PARAMS = sizeof(marketOrderParams) / sizeof(DoubleParamDesc);




// --- Fonction factorisée d'affichage UI ---
void DrawModelParametersUI(SimuParams& params, const SimuParamBounds& bounds) {
    if (ImGui::Begin("Paramètres Simulation")) {

        if (ImGui::CollapsingHeader("Add Liquidity")) {
            if (ImGui::TreeNode("Price Distribution")) {
                for (int i = 0; i < NB_ADDLIQ_PARAMS; ++i) {
                    ImGui::SliderScalar(addLiqPriceDistParams[i].label, ImGuiDataType_Double,
                        addLiqPriceDistParams[i].value,
                        &addLiqPriceDistParams[i].bounds->min,
                        &addLiqPriceDistParams[i].bounds->max,
                        addLiqPriceDistParams[i].format);
                }
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Remove Liquidity")) {
            for (int i = 0; i < NB_REMOVELIQ_PARAMS; ++i) {
                ImGui::SliderScalar(removeLiqParams[i].label, ImGuiDataType_Double,
                    removeLiqParams[i].value,
                    &removeLiqParams[i].bounds->min,
                    &removeLiqParams[i].bounds->max,
                    removeLiqParams[i].format);
            }
        }

        if (ImGui::CollapsingHeader("Market Order")) {
            for (int i = 0; i < NB_MARKETORDER_PARAMS; ++i) {
                ImGui::SliderScalar(marketOrderParams[i].label, ImGuiDataType_Double,
                    marketOrderParams[i].value,
                    &marketOrderParams[i].bounds->min,
                    &marketOrderParams[i].bounds->max,
                    marketOrderParams[i].format);
            }
        }
    }
    ImGui::End();
}