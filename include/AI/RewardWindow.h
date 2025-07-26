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
    torch::Tensor best_asks;
    torch::Tensor best_bids;
    torch::Tensor agent_state_tensor;

    bool isInvalid;

    RewardWindow(int idx, Action act, float price, int state, const torch::Tensor& p,
        const torch::Tensor& v, const torch::Tensor& s,
        const torch::Tensor& a, const torch::Tensor& b,
        bool invalid = false)
        : decision_index(idx), action(act), proba(p), value(v), entry_price(price),
        agent_state_at_action(state), isInvalid(invalid),
        latent_pnls(torch::empty({ 0 }, torch::kFloat).to(torch::kCUDA)),
        agent_state_tensor(s), best_asks(a), best_bids(b) {
    }

    bool isComplete() const {
        return latent_pnls.size(0) >= 100; // ou autre valeur N
    }

    torch::Tensor computeWeightedReward(float alpha = 0.2f) const {
        
        if (agent_state_at_action == 0 && action == Action::WAIT) {
            //std::cout << "WAIT with no position, reward: -0.01" << std::endl;
            return torch::tensor({ -2.0f }, torch::kFloat).to(torch::kCUDA);
        }
        if (isInvalid) {
            //std::cout << "Invalid action, reward: -0.01" << std::endl;
            return torch::tensor({ -10.0f }, torch::kFloat).to(torch::kCUDA);
        }
        if (latent_pnls.size(0) == 0) {
            //std::cout << "Empty latent_pnls, reward: 0" << std::endl;
            return torch::tensor({ 0.0f }, torch::kFloat).to(torch::kCUDA);
        }
        auto indices = torch::arange(latent_pnls.size(0) - 1, -1, -1, torch::kFloat).to(torch::kCUDA);
        auto weights = torch::exp(-alpha * indices);
        auto total_weight = weights.sum();
        auto reward = (weights * latent_pnls).sum();
        auto weighted_reward = total_weight.item<float>() > 0.0f
            ? (reward / total_weight).unsqueeze(0)
            : torch::tensor({ 0.0f }, torch::kFloat).to(torch::kCUDA);
        return weighted_reward;
    }

    void addPnL(float best_bid, float best_ask) {
        if (latent_pnls.size(0) >= 100) {
            //std::cout << "RewardWindow full, skipping addPnL" << std::endl;
            return;
        }
        float pnl = 0.0f;

        if (action == Action::WAIT) {
            if (agent_state_at_action == 0) { pnl = 0.0f; }
            else if (agent_state_at_action == 1) { pnl = best_bid - entry_price; }
            else if (agent_state_at_action == -1) { pnl = entry_price - best_ask; }
        }

        else if (action == Action::BUY_MARKET) {
            if (agent_state_at_action <= 0) { pnl = best_bid - entry_price; }
        }

        else if (action == Action::SELL_MARKET) {
            if (agent_state_at_action >= 0) { pnl = entry_price - best_ask; }
        }

        auto new_pnl = torch::tensor({ pnl }, torch::kFloat).to(torch::kCUDA);
        latent_pnls = torch::cat({ latent_pnls, new_pnl });
    }
};

