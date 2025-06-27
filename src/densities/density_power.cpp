#include "densities/density_power.h"
#include <vector>
#include "engine/OrderBook.h"
#include <iostream>
#include <numeric>

std::vector<double> density_power(
	const Side& side,
	const double currentBestBid,
	const double currentBestAsk,
	const std::vector<double>& prices,
	const SimuParams& params
) {
	std::vector<double> f_density(prices.size(), 0.0);

	double alpha = params.addLiq.priceDist.powDistParam;
	if (side == Side::ASK) {
		for (int i = 0; i < prices.size(); ++i) {
			if (prices[i] <= currentBestBid) {
				f_density[i] = std::pow(1.0 / (currentBestBid - prices[i] + ticksize), alpha);
			}
			else {
				f_density[i] = std::pow(1.0 / (prices[i] - currentBestBid), alpha);
			}
		}
	}
	else {
		for (int i = 0; i < prices.size(); ++i) {
			if (prices[i] < currentBestAsk) {
				f_density[i] = std::pow(1.0 / (currentBestAsk - prices[i]), alpha);
			}
			else {
				f_density[i] = std::pow(1.0 / (prices[i] - currentBestAsk + ticksize), alpha);
			}
		}
	}

	double sum = std::accumulate(f_density.begin(), f_density.end(), 0.0);
	for (double& d : f_density) d /= sum;

	return f_density;
}

