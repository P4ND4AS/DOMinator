#include "densities/densities_add_liq.h"
#include "engine/OrderBook.h"
#include "engine/SimuParams.h"
#include "densities/density_brownian.h"
#include "densities/density_power.h"
#include "densities/density_utils.h"

double sampleLambdaL(SimuParams& params, double S, double q1Ask, double q1Bid, std::mt19937& rng) {
	 double p_add_liq = 0.005;
	 return p_add_liq;
}



Side sampleAddLiqSide(SimuParams& params, double S, double q1Ask, double q1Bid,
				      std::mt19937& rng) {
	double p_bid = 0.5;
	std::bernoulli_distribution side_dist(p_bid);
	return side_dist(rng) ? Side::BID : Side::ASK;
}




int sampleAddLiqSize(SimuParams& params, std::mt19937& rng) {
	std::exponential_distribution<> size_dist(1.0);
	int size = (int)size_dist(rng) + 1;
	return size;
}



double sampleAddLiqPrice(SimuParams& params, Side side, double currentBestBid,
	double currentBestAsk, double minPrice, double maxPrice, std::vector<double> prices,
	std::vector<Foyer>& foyers_state, std::mt19937& rng) {

	// 1. Construction de valid_prices
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


	const double& amplitudeBrownian = params.addLiq.priceDist.amplitudeBrownian;
	const double& powDistParam = params.addLiq.priceDist.powDistParam;

	// 2. Calcul des différentes densités sur le prix.
	std::vector<double> brownian_density = density_brownian(foyers_state, prices, amplitudeBrownian);
	std::vector<double> power_density = density_power(side, currentBestBid, currentBestAsk,
													  prices, powDistParam);


	// 3. Projection sur valid_prices
	std::vector<double> brownian_valid, power_valid;
	for (double price : valid_prices) {
		int idx = static_cast<int>((price - minPrice) / ticksize);
		brownian_valid.push_back(brownian_density[idx]);
		power_valid.push_back(power_density[idx]);
	}

	// 4. Combinaison pondérée et normalisation
	double w_brown = params.addLiq.priceDist.weight_brownian;
	double w_power = params.addLiq.priceDist.weight_power;

	std::vector<std::vector<double>> densities = { brownian_valid, power_valid };
	std::vector<double> weights = { w_brown, w_power };
	std::vector<double> total_density = combine_and_normalize_densities(densities, weights);

	// 5. Sampling selon la densité combinée
	std::discrete_distribution<> price_dist(total_density.begin(), total_density.end());
	int idx = price_dist(rng);
	return valid_prices[idx];
}
