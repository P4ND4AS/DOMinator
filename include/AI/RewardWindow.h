#pragma once
#include <Eigen/Dense>
#include <torch/torch.h>

enum Action { BUY_MARKET = 0, SELL_MARKET = 1, WAIT = 2 };

struct AgentState {
    int position = 0;

    Eigen::VectorXf toVector() const {
        Eigen::VectorXf vec(1);
        vec << position;
        return vec;
    }

    torch::Tensor toTensor() const {
        return torch::tensor({ static_cast<float>(position) }, torch::kFloat).to(torch::kCUDA);
    }
};

struct RewardWindow {
    int decision_index;
    Action action;
    torch::Tensor proba;
    torch::Tensor value;
    float entry_price;
    int agent_state_at_action;
    torch::Tensor latent_pnls;
    torch::Tensor heatmap;
    torch::Tensor agent_state_tensor;

    bool isInvalid;

    RewardWindow(int idx, Action act, float price, int state, const torch::Tensor& p,
        const torch::Tensor& v, const torch::Tensor& h, const torch::Tensor& s,
        bool invalid = false)
        : decision_index(idx), action(act), proba(p), value(v), entry_price(price),
        agent_state_at_action(state), isInvalid(invalid),
        latent_pnls(torch::empty({ 0 }, torch::kFloat).to(torch::kCUDA)),
        heatmap(h), agent_state_tensor(s) {
    }

    bool isComplete() const {
        return latent_pnls.size(0) >= 10; // ou autre valeur N
    }

    torch::Tensor computeWeightedReward(float alpha = 0.2f) const {
        if (agent_state_at_action == 0 && action == Action::WAIT) {
            return torch::tensor({ -0.01f }, torch::kFloat).to(torch::kCUDA);
        }
        if (isInvalid) {
            return torch::tensor({ -0.01f }, torch::kFloat).to(torch::kCUDA);
        }

        if (latent_pnls.size(0) == 0) {
            return torch::tensor({ 0.0f }, torch::kFloat).to(torch::kCUDA);
        }

        // Calculer les poids : exp(-alpha * [N-1, N-2, ..., 0])
        auto indices = torch::arange(latent_pnls.size(0) - 1, -1, -1, torch::kFloat).to(torch::kCUDA);
        auto weights = torch::exp(-alpha * indices);
        float total_weight = weights.sum().item<float>();

        // Somme pondérée
        auto reward = (weights * latent_pnls).sum();

        return total_weight > 0.0f ? reward / total_weight : torch::tensor({ 0.0f }, torch::kFloat).to(torch::kCUDA);
    }

    void addPnL(float best_bid, float best_ask) {

        float pnl = 0.0f;
        if (agent_state_at_action == 0 && action == Action::WAIT) {
            pnl = 0.0f;
        }
        else if (agent_state_at_action > 0) {
            pnl = best_bid - entry_price;
        }
        else if (agent_state_at_action < 0) {
            pnl = entry_price - best_ask;
        }

        // Ajouter le PnL au tenseur
        auto new_pnl = torch::tensor({ pnl }, torch::kFloat).to(torch::kCUDA);
        latent_pnls = torch::cat({ latent_pnls, new_pnl });
    }
};

