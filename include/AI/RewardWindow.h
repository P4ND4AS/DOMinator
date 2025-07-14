#pragma once
#include "TradingAI.h"
#include <Eigen/Dense>

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
    float entry_price;  // utilisé seulement si in_position == true
    bool in_position;
    std::vector<float> latent_pnls;

    RewardWindow(int idx, Action act, float price, bool in_pos, float proba, float value)
        : decision_index(idx), action(act), entry_price(price), in_position(in_pos), 
          proba(proba), value(value) {
    }

    bool isComplete() const {
        return latent_pnls.size() >= 10;
    }

    float computeWeightedReward() const {
        if (!in_position && action == WAIT) return 0.0f; // rien à évaluer
        float reward = 0.0f;
        float total_weight = 0.0f;
        for (int i = 0; i < latent_pnls.size(); ++i) {
            float weight = 1.0f; // poids uniforme pour l’instant
            reward += weight * latent_pnls[i];
            total_weight += weight;
        }
        return (total_weight > 0.0f) ? reward / total_weight : 0.0f;
    }

    void addPnL(float best_bid, float best_ask, const AgentState& state) {
        if (state.position == 0 && action == WAIT) {
            latent_pnls.push_back(0.0f);
            return;
        }

        float pnl = 0.0f;
        if (state.position > 0) { // long
            pnl = best_bid - entry_price;
        }
        else if (state.position < 0) { // short
            pnl = entry_price - best_ask;
        }

        latent_pnls.push_back(pnl);
    }


};
