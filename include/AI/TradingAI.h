#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"
#include "RewardWindow.h"
#include "NeuralNetwork.h"
#include <random>
#include <fstream>
#include <filesystem>


// Represents either a single iteration during data collecting OR a batch during optimization
struct Transition {
    torch::Tensor best_asks;
    torch::Tensor best_bids;
    torch::Tensor agent_state;
    torch::Tensor action;
    torch::Tensor log_prob;
    torch::Tensor reward;
    torch::Tensor value;
    torch::Tensor done;
};



class MemoryBuffer {
public:
    MemoryBuffer(int64_t T_max); // Constructeur avec taille maximale
    void store(const Transition& exp);
    void clear();
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor,
        torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> get() const;

    torch::Tensor computeReturns(float last_value, float gamma) const;
    torch::Tensor computeAdvantages(float last_value, float gamma, float lambda) const;

    Transition sampleMiniBatch(int batch_size, const torch::Tensor& indices) const;

private:
    int64_t T_max_;              // Taille maximale de la trajectoire
    int64_t current_size_;       // Nombre de transitions stockées
    torch::Tensor best_asks;     // [T_max, 1, 50]
    torch::Tensor best_bids;     // [T_max, 1, 50]
    torch::Tensor agent_states;  // [T_max, 1]
    torch::Tensor actions;       // [T_max]
    torch::Tensor log_probs;     // [T_max]
    torch::Tensor rewards;       // [T_max]
    torch::Tensor values;        // [T_max]
    torch::Tensor dones;         // [T_max]
};


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------



struct TradeLog {
    int timestep_entry;
    float price_entry;
    int timestep_exit;
    float price_exit;
    float pnl;
    Action entry_action;
    Action exit_action;
};


class TradingEnvironment {
public:

    TradingEnvironment(TradingAgentNet* network, 
        std::mt19937 rng, int N_trajectories = 1,
        int traj_duration = 10, int decision_per_second = 10);

    ~TradingEnvironment();

    void reset();
    Action sampleFromPolicy(const torch::Tensor& policy, std::mt19937& rng);

    void handleAction(Action action, const torch::Tensor& log_prob, const torch::Tensor& value);
    void updateRewardWindows();

	AgentState getAgentState() const { return agent_state; }
    MemoryBuffer getMemoryBuffer() const { return memoryBuffer; }

    void printTradeLogs() const;
    void computeMetrics(int episode);

    void collectTransitions(std::mt19937& rng);
    void optimize(std::mt19937& rng, int num_epochs, int batch_size, float clip_param,
        float value_loss_coeff, float entropy_coef);
    void train(int num_trajectories, int num_epochs, int batch_size, float clip_param = 0.2f, float value_loss_coef = 0.25f, float entropy_coef = 0.02f);

    int current_decision_index = 0;
private:
    OrderBook orderBook;
    Heatmap heatmap;       
    torch::Tensor best_asks_tensor;
    torch::Tensor best_bids_tensor;
    TradingAgentNet* network;
    MemoryBuffer memoryBuffer;
    torch::optim::Adam* optimizer;

    std::mt19937 rng;

    AgentState agent_state;
    std::vector<RewardWindow> reward_windows;
    std::vector<TradeLog> trade_logs;
    std::optional<TradeLog> open_trade;
    float entry_price = 0.0f;

    
    int decision_per_second;
    int traj_duration;
    int marketUpdatePerDecision = static_cast<int>(1 / (decision_per_second * timestep * 0.000001f));
    bool isEpisodeDone = false;
};