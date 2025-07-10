#pragma once
#include <vector>
#include "engine/OrderBook.h"

enum Action { BUY_MARKET, CANCEL_ORDER, WAIT };

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

private:
	std::vector<Experience> buffer;
};