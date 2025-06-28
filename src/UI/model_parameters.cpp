#include "UI/model_parameters.h"
#include "imgui.h"

void DrawModelParametersUI(SimuParams& params) {
    if (ImGui::Begin("Paramètres Simulation")) {

        if (ImGui::CollapsingHeader("Add Liquidity")) {
            if (ImGui::TreeNode("Price Distribution")) {
                ImGui::InputDouble("mu_jitter", &params.addLiq.priceDist.mu_jitter, 0.01, 0.1, "%.6f");
                ImGui::InputDouble("mu_init", &params.addLiq.priceDist.mu_init);
                ImGui::InputDouble("p_birth", &params.addLiq.priceDist.p_birth, 0.0f, 0.001f, "%.6f");
                ImGui::InputDouble("p_death", &params.addLiq.priceDist.p_death, 0.0f, 0.001f, "%.6f");
                ImGui::InputDouble("sigma_jitter", &params.addLiq.priceDist.sigma_jitter, 0.0f, 0.3f);
                ImGui::InputDouble("sigma_init", &params.addLiq.priceDist.sigma_init);
                ImGui::InputDouble("amplitudeBrownian", &params.addLiq.priceDist.amplitudeBrownian, 0.0f, 2.0f);
                ImGui::InputDouble("powDistParam", &params.addLiq.priceDist.powDistParam, 0.0f, 2.0f);
                ImGui::InputDouble("weight_brownian", &params.addLiq.priceDist.weight_brownian, 0.0f, 1.0f);
                ImGui::InputDouble("weight_power", &params.addLiq.priceDist.weight_power, 0.0f, 1.0f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Remove Liquidity")) {
            ImGui::InputDouble("mu_jitter##remove", &params.removeLiq.mu_jitter, 0.0f, 0.2f);
            ImGui::InputDouble("mu_init##remove", &params.removeLiq.mu_init);
            ImGui::InputDouble("p_birth##remove", &params.removeLiq.p_birth, 0.0f, 0.001f, "%.6f");
            ImGui::InputDouble("p_death##remove", &params.removeLiq.p_death, 0.0f, 0.001f, "%.6f");
            ImGui::InputDouble("sigma_jitter##remove", &params.removeLiq.sigma_jitter, 0.0f, 0.3f);
            ImGui::InputDouble("sigma_init##remove", &params.removeLiq.sigma_init);
            ImGui::InputDouble("amplitudeBrownian##remove", &params.removeLiq.amplitudeBrownian, 0.0f, 2.0f);
            ImGui::InputDouble("powDistParam##remove", &params.removeLiq.powDistParam, 0.0f, 2.0f);
            ImGui::InputDouble("weight_brownian##remove", &params.removeLiq.weight_brownian, 0.0f, 1.0f);
            ImGui::InputDouble("weight_power##remove", &params.removeLiq.weight_power, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Market Order")) {
            ImGui::InputDouble("e", &params.marketOrder.e, 0.0f, 1.0f);
        }
    }

    ImGui::End();
}