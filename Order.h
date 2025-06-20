#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <chrono>
#include <optional>


enum class Side { BID, ASK };

struct LimitOrder {
	uint64_t id;
	std::string timestamp;
	double price;
	int size;
	Side side;

	LimitOrder() {}
};


struct MarketOrder {
	std::string timestamp;
	Side side;
	int size;
};