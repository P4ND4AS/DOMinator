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
	const double& powDistParam

) {
	std::vector<double> f_density(prices.size(), 0.0);

	double alpha = powDistParam;
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

std::vector<double> density_power_globale(
	const double currentBestBid,
	const double currentBestAsk,
	const std::vector<double>& prices,
	const double& powDistParam

) {

	std::vector<double> f_density(prices.size(), 0.0);

	double alpha = powDistParam;

	std::vector<double> bidSideDensity(prices.size(), 0.0);
	std::vector<double> askSideDensity(prices.size(), 0.0);


	for (int i = 0; i < prices.size(); ++i) {
		if (prices[i] <= currentBestBid) {
			bidSideDensity[i] = std::pow(1.0 / (currentBestBid - prices[i] + ticksize), alpha);
		}
		else {
			bidSideDensity[i] = std::pow(1.0 / (prices[i] - currentBestBid), alpha);
		}
	}
	for (int i = 0; i < prices.size(); ++i) {
		if (prices[i] < currentBestAsk) {
			askSideDensity[i] = std::pow(1.0 / (currentBestAsk - prices[i]), alpha);
		}
		else {
			askSideDensity[i] = std::pow(1.0 / (prices[i] - currentBestAsk + ticksize), alpha);
		}
	}
	for (int i = 0; i < prices.size(); ++i) {
		f_density[i] = bidSideDensity[i] + askSideDensity[i];
	}
	return f_density;
}