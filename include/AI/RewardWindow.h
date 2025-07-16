#pragma once
#include "TradingAI.h"
#include <Eigen/Dense>
/*
enum Action { BUY_MARKET = 0, SELL_MARKET = 1, WAIT = 2 };

struct AgentState {
    int position = 0;

    Eigen::VectorXf toVector() const {
        Eigen::VectorXf vec(1);
        vec << position;
        return vec;
    }
};

struct RewardWindow {
    int decision_index;
    Action action;
    float proba;
    float value;
    float entry_price;
    int agent_state_at_action;
    std::vector<float> latent_pnls;

    bool isInvalid;

    RewardWindow(int idx, Action act, float price, int state, float p, float v, bool invalid = false)
        : decision_index(idx), action(act), proba(p), value(v),
        entry_price(price), agent_state_at_action(state), isInvalid(invalid) {
    }

    bool isComplete() const {
        return latent_pnls.size() >= 10;  // ou autre valeur N
    }

    float computeWeightedReward(float alpha = 0.2f) const {
        if (agent_state_at_action == 0 && action == WAIT)
            return -0.01f;
        if (isInvalid) { return -0.01f; }

        float reward = 0.0f;
        float total_weight = 0.0f;

        for (size_t i = 0; i < latent_pnls.size(); ++i) {
            float weight = std::exp(-alpha * (latent_pnls.size() - 1 - i));
            reward += weight * latent_pnls[i];
            total_weight += weight;
        }

        return (total_weight > 0.0f) ? reward / total_weight : 0.0f;
    }

    void addPnL(float best_bid, float best_ask) {
        if (agent_state_at_action == 0 && action == WAIT) {
            latent_pnls.push_back(0.0f);
            return;
        }

        float pnl = 0.0f;
        if (agent_state_at_action > 0) {
            pnl = best_bid - entry_price;
        }
        else if (agent_state_at_action < 0) {
            pnl = entry_price - best_ask;
        }

        latent_pnls.push_back(pnl);
    }
};*/

