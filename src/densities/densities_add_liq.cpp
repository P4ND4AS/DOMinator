#include "densities/densities_add_liq.h"
#include "engine/OrderBook.h"


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
	double currentBestAsk, double minPrice, double maxPrice,
	std::mt19937& rng) {

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

	double lambda = params.addLiq.a;
	std::exponential_distribution<> exp_dist(lambda);
	int idx = std::clamp((int)exp_dist(rng), 0, (int)valid_prices.size() - 1);
	return valid_prices[idx];
}
