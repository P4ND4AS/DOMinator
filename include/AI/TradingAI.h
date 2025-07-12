#pragma once
#include <vector>
#include "engine/OrderBook.h"
#include "Heatmap.h"

enum Action { BUY_MARKET, SELL_MARKET, WAIT };

int HEATMAP_ROWS = 2 * depth + 1;
int HEATMAP_COLS = 300;
constexpr int N_ACTIONS = 3;

constexpr float GAMMA = 0.99f;
constexpr float LAMBDA = 0.95f;
constexpr float CLIP_EPS = 0.2;

struct Experience {
	std::vector<std::vector<float>> state;
	Action action;
	float reward;
	bool done;
	std::vector<std::vector<float>> nextState;
	float value;
	float logProb;
};


class MemoryBuffer {
public:
	void store(const Experience& exp);
	void clear();
	const std::vector<Experience>& get() const;

	std::vector<float> computeAdvantages(float lastValue);
	std::vector<float> computeReturns(float lastValue);

private:
	std::vector<Experience> buffer;
};


// --------------------- TRADING ENVIRONMENT FOR TRAINING ---------------------

#include "NeuralNetwork.h"
#include <random>

class TradingEnvironment {
public:
    TradingEnvironment(OrderBook* book);

    void reset();
    void updateMarket(int n_iter, std::mt19937& rng);

    std::vector<std::vector<float>> getObservation();
    Eigen::VectorXf getAgentState() const;

    float step(Action action, std::mt19937& rng);
    bool isDone() const;
    float getCumulativeReward() const;

private:
    OrderBook* orderBook;
    Heatmap heatmap;
    int timestep;
    const int maxTimesteps;
    bool episodeDone;
    float cumulativeReward;

    int currentPosition;   // -1 short, 0 flat, 1 long
    double entryPrice = 0.0;
};