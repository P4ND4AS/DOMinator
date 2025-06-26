#include "densities/densities_add_liq.h"
#include "engine/OrderBook.h"
#include "engine/SimuParams.h"

double sampleLambdaL(const SimuParams& params, double S, double q1Ask, double q1Bid, std::mt19937& rng) {
	 double p_add_liq = 0.005;
	 return p_add_liq;
}



Side sampleAddLiqSide(const SimuParams& params, double S, double q1Ask, double q1Bid,
				      std::mt19937& rng) {
	double p_bid = 0.5;
	std::bernoulli_distribution side_dist(p_bid);
	return side_dist(rng) ? Side::BID : Side::ASK;
}




int sampleAddLiqSize(const SimuParams& params, std::mt19937& rng) {
	std::exponential_distribution<> size_dist(1.0);
	int size = (int)size_dist(rng) + 1;
	return size;
}



double sampleAddLiqPrice(const SimuParams& params, Side side, double currentBestBid,
	double currentBestAsk, double minPrice, double maxPrice, std::vector<double> prices,
	std::vector<Foyer>& foyers_state, std::mt19937& rng) {

	std::vector<double> valid_prices;
	if (side == Side::BID) {
		for (double p = currentBestAsk - ticksize; p >= minPrice; p -= ticksize) {
			valid_prices.push_back(p);
		}
	}
	else {
		for (double p = currentBestBid + ticksize; p <= maxPrice; p += ticksize) {
			valid_prices.push_back(p);
		}
	}

	std::vector<double> density = density_brownian(foyers_state, prices, params);


	std::vector<double> density_valid;
	for (double price : valid_prices) {
		int idx = static_cast<int>((price - minPrice) / ticksize);
		density_valid.push_back(density[idx]);
	}

	std::discrete_distribution<> price_dist(density_valid.begin(), density_valid.end());
	int idx = price_dist(rng);
	return valid_prices[idx];
}
