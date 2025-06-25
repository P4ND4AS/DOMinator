#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <chrono>
#include <optional>
#include <cstdint>


enum class Side { BID, ASK };

struct LimitOrder {
	uint64_t id;
	int64_t timestamp;
	double price;
	int size;
	Side side;

	LimitOrder() {}
};


struct MarketOrder {
	int64_t timestamp;
	Side side;
	int size;
};