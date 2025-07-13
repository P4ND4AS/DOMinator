#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"
#include "RewardWindow.h"


int HEATMAP_ROWS = 2 * depth + 1;
int HEATMAP_COLS = 300;
constexpr int N_ACTIONS = 3;

constexpr float GAMMA = 0.99f;
constexpr float LAMBDA = 0.95f;
constexpr float CLIP_EPS = 0.2;

struct Transition {
    std::vector<float> observation;  // Flattened heatmap at t (peut être un pointeur ou index si trop gros)
    Action action;                   // Action prise à t
    float log_prob;                  // log(pi_theta(a_t | s_t)) pour l’ancienne politique
    float value;                     // estimation V(s_t) donnée par l’ancien réseau
    float reward;                    // récompense calculée a posteriori
    bool done;                       // true si fin de trajectoire
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
    void handleAction(Action action);
    void updateRewardWindows();

	AgentState getAgentState() const { return agent_state; }

    void printTradeLogs() const;

    void train(std::mt19937& rng);

private:
    OrderBook* orderBook;
    Heatmap heatmap;
    PolicyValueNet* network;
    MemoryBuffer buffer;

    AgentState agent_state;
    std::vector<RewardWindow> reward_windows;
    std::vector<TradeLog> trade_logs;
    std::optional<TradeLog> open_trade;

    int current_decision_index = 0;
    int decision_per_second = 10;
    int traj_duration = 600;
    int marketUpdatePerDecision = 1 / (decision_per_second * timestep / 1000000);
    int N_trajectories = 1;
	bool isEpisodeDone = false;
};