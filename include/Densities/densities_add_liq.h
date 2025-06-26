#pragma once
#include <vector>
#include <random>
#include "engine/Order.h"
#include "engine/SimuParams.h"

double sampleLambdaL(const SimuParams& params, double S, double q1Ask, 
					 double q1Bid, std::mt19937& rng);


Side sampleAddLiqSide(const SimuParams& params, double S, double q1Ask, 
				     double q1Bid, std::mt19937& rng);


int sampleAddLiqSize(const SimuParams& params, std::mt19937& rng);

double sampleAddLiqPrice(const SimuParams& params, Side side, double currentBestBid,
						 double currentBestAsk, double minPrice, double maxPrice, 
						 std::mt19937& rng);