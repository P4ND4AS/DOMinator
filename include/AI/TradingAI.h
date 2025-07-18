#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"
#include "RewardWindow.h"
#include "NeuralNetwork.h"
#include <random>



struct Transition {
    torch::Tensor heatmap;     
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
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor,
        torch::Tensor, torch::Tensor, torch::Tensor> get() const;

    torch::Tensor computeReturns(float last_value, float gamma) const;
    torch::Tensor computeAdvantages(float last_value, float gamma, float lambda) const;

    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor,
        torch::Tensor, torch::Tensor, torch::Tensor>
        sampleMiniBatch(int batch_size, std::mt19937& rng) const;

private:
    int64_t T_max_;              // Taille maximale de la trajectoire
    int64_t current_size_;       // Nombre de transitions stockées
    torch::Tensor heatmaps;      // [T_max, 1, 401, 800]
    torch::Tensor agent_states;  // [T_max]
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

    TradingEnvironment(OrderBook* book, TradingAgentNet* network, int N_trajectories = 1,
        int traj_duration = 10, int decision_per_second = 10);

    Action sampleFromPolicy(const torch::Tensor& policy, std::mt19937& rng);

    void handleAction(Action action, const torch::Tensor& log_prob, const torch::Tensor& value);
    void updateRewardWindows();

	AgentState getAgentState() const { return agent_state; }
    torch::Tensor getHeatmapTensor() const { return heatmap_data_tensor; }
    MemoryBuffer getMemoryBuffer() const { return memoryBuffer; }

    void printTradeLogs() const;
    void train(std::mt19937& rng);

private:
    OrderBook* orderBook;
    Heatmap heatmap;       
    torch::Tensor heatmap_data_tensor;
    TradingAgentNet* network;
    MemoryBuffer memoryBuffer;

    AgentState agent_state;
    std::vector<TradeLog> trade_logs;
    std::optional<TradeLog> open_trade;
    float entry_price = 0.0f;

    int current_decision_index = 0;
    int decision_per_second;
    int traj_duration;
    int marketUpdatePerDecision;
    bool isEpisodeDone = false;
};