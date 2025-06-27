#pragma once
#include <vector>
#include "density_utils.h"
#include "engine/Order.h"
#include "density_brownian.h"


double sampleRemoveLiqPrice(const SimuParams& params, double currentBestBid,
	double currentBestAsk, double minPrice, double maxPrice,
	std::vector<double> prices, std::vector<Foyer>& foyers_state,
	std::mt19937& rng);