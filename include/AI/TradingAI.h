#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"
#include "RewardWindow.h"


constexpr int N_ACTIONS = 3;

constexpr float GAMMA = 0.99f;
constexpr float LAMBDA = 0.95f;
constexpr float CLIP_EPS = 0.2f;

struct Transition {
    std::vector<float> observation;  
    int agent_state;
    Action action;                   
    float log_prob;                  
    float value;                     
    float reward;                    
    bool done;      
};



class MemoryBuffer {
public:
	void store(const Transition& exp);
	void clear();
	const std::vector<Transition>& get() const;

	std::vector<float> computeAdvantages(float lastValue);
	std::vector<float> computeReturns(float lastValue);

private:
	std::vector<Transition> buffer;
};


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------

#include "NeuralNetwork.h"
#include <random>

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
};