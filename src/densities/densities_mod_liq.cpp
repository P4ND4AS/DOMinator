#include "densities/densities_mod_liq.h"
#include "densities/density_power.h"
#include "densities/density_brownian.h"


double sampleRemoveLiqPrice(const SimuParams& params, double currentBestBid,
	double currentBestAsk, double minPrice, double maxPrice,
	std::vector<double> prices, std::vector<Foyer>& foyers_state,
	std::mt19937& rng) {

	const double& amplitudeBrownian = params.removeLiq.amplitudeBrownian;
	const double& powDistParam = params.removeLiq.powDistParam;

	// 1. Calcul des densités sur tout prices
	std::vector<double> brownian_density = density_brownian(foyers_state, prices, amplitudeBrownian);
	std::vector<double> power_density = density_power_globale(currentBestBid, currentBestAsk,
		prices, powDistParam);

	// 2. Combinaison pondérée et normalisation
	double w_brownian = params.removeLiq.weight_brownian;
	double w_power = params.removeLiq.weight_power;

	std::vector<std::vector<double>> densities = { brownian_density, power_density };
	std::vector<double> weights = { w_brownian, w_power };

	std::vector<double> total_density = combine_and_normalize_densities(densities, weights);

	// 3. Sampling selon la densité combinée
	std::discrete_distribution<> price_dist(total_density.begin(), total_density.end());
	int idx = price_dist(rng);
	return prices[idx];
}
