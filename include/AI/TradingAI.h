#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"
#include "RewardWindow.h"
#include "NeuralNetwork.h"
#include <torch/torch.h>
#include <random>



struct Transition {
    torch::Tensor heatmap;      // Heatmap (1, 401, 800)
    torch::Tensor agent_state;  // État de l'agent (1)
    torch::Tensor action;       // Action choisie
    torch::Tensor log_prob;     // Log-probabilité de l'action
    torch::Tensor reward;       // Récompense
    torch::Tensor value;        // Valeur estimée
    torch::Tensor done;         // Indicateur de fin d'épisode
};



class MemoryBuffer {
public:
    void store(const Transition& exp);
    void clear();
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, 
        torch::Tensor, torch::Tensor, torch::Tensor> get() const;

    // Compute GAE
    torch::Tensor computeAdvantages(float last_value, float gamma, float lambda) const;
    // Compute returns
    torch::Tensor computeReturns(float last_value, float gamma) const;

    // Sample mini-batch
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, 
        torch::Tensor, torch::Tensor, torch::Tensor>
        sampleMiniBatch(int batch_size, std::mt19937& rng) const;

private:
    // Tensors for transitions
    std::vector<torch::Tensor> heatmaps;     
    std::vector<torch::Tensor> agent_states; 
    std::vector<torch::Tensor> actions;       
    std::vector<torch::Tensor> log_probs;     
    std::vector<torch::Tensor> rewards;       
    std::vector<torch::Tensor> values;        
    std::vector<torch::Tensor> dones;         
};


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------
/*


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

    TradingEnvironment(OrderBook* book, PolicyValueNet* network);

    Action sampleFromPolicy(const Eigen::VectorXf& policy, std::mt19937& rng);
    void handleAction(Action action, const Eigen::VectorXf& policy, const float value);
    void updateRewardWindows();

	AgentState getAgentState() const { return agent_state; }

    void printTradeLogs() const;

    void train(std::mt19937& rng);

private:
    OrderBook* orderBook;
    Heatmap heatmap;
    PolicyValueNet* network;
    MemoryBuffer memoryBuffer;

    AgentState agent_state;
    std::vector<RewardWindow> reward_windows;
    std::vector<TradeLog> trade_logs;
    std::optional<TradeLog> open_trade;
    float entry_price = 0.0f;

    int current_decision_index = 0;
    int decision_per_second = 10;
    int traj_duration = 10;
    int marketUpdatePerDecision = 1 / (decision_per_second * timestep / 1000000.0f);
    int N_trajectories = 1;
	bool isEpisodeDone = false;
};*/