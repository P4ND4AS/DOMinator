#include "densities/density_utils.h"
#include <numeric>
#include <cassert>

std::vector<double> combine_and_normalize_densities(
	const std::vector<std::vector<double>>& densities,
	const std::vector<double>& weights
) {
	assert(!densities.empty());
	size_t n = densities[0].size();
	for (const auto& d : densities) {
		assert(d.size() == n); // Toutes les densités doivent faire la même taille
	}

	std::vector<double> w = weights;
	if (w.empty()) {
		w = std::vector<double>(densities.size(), 1.0);
	}
	assert(w.size() == densities.size());

	// Somme pondérée
	std::vector<double> result(n, 0.0);
	for (size_t d = 0; d < densities.size(); ++d) {
		for (size_t i = 0; i < n; ++i) {
			result[i] += w[d] * densities[d][i];
		}
	}

	// Normalisation
	double sum = std::accumulate(result.begin(), result.end(), 0.0);
	if (sum > 0) {
		for (double& v : result) v /= sum;
	}
	return result;
}