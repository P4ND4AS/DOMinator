#pragma once
#include <vector>
#include <random>
#include "engine/Order.h"
#include "engine/SimuParams.h"
#include "densities/density_brownian.h"

double sampleLambdaL(SimuParams& params, double S, double q1Ask, 
					 double q1Bid, std::mt19937& rng);


Side sampleAddLiqSide(SimuParams& params, double S, double q1Ask, 
				     double q1Bid, std::mt19937& rng);


int sampleAddLiqSize(SimuParams& params, std::mt19937& rng);

double sampleAddLiqPrice(SimuParams& params, Side side, double currentBestBid,
						 double currentBestAsk, double minPrice, double maxPrice, 
						 std::vector<double> prices, std::vector<Foyer>& foyers_state,
						 std::mt19937& rng);