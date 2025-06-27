#include <random>
#include "densities/density_brownian.h"
#include "engine/OrderBook.h"
#include <cmath>	
#include <algorithm>
#include <iostream>
#include <numeric>


void updateFoyerState(
	std::vector<Foyer>& foyers_state,
	const std::vector<double>& prices,
	std::mt19937& rng,
	const SimuParams& params
) {
	// Supprimer les foyers morts
    std::bernoulli_distribution death_dist(params.addLiq.priceDist.p_death);
    foyers_state.erase(
        std::remove_if(
            foyers_state.begin(), foyers_state.end(),
            [&](const Foyer&) { return death_dist(rng); }
        ),
        foyers_state.end()
    );

    // Ajouter un nouveau foyer ?
	std::bernoulli_distribution birth_dist(params.addLiq.priceDist.p_birth);
    if (birth_dist(rng)) {
        std::uniform_int_distribution<> pick_price(0, prices.size() - 1);
        double new_mu = prices[pick_price(rng)];
        foyers_state.push_back({ new_mu, params.addLiq.priceDist.sigma_init });
    }

    // Faire évoluer chaque foyer
    std::normal_distribution<> mu_noise(0.0, params.addLiq.priceDist.mu_jitter);
    std::normal_distribution<> sigma_noise(0.0, params.addLiq.priceDist.sigma_jitter);

    for (auto& foyer : foyers_state) {
        foyer.mu += mu_noise(rng);
        foyer.mu = std::clamp(foyer.mu, prices.front(), prices.back());
        foyer.sigma += sigma_noise(rng);
        foyer.sigma = std::max(0.1, foyer.sigma);

    }
}



// Calcule la densité brownienne sur les prix
std::vector<double> density_brownian(
    const std::vector<Foyer>& foyers_state,
    const std::vector<double>& prices,
    const SimuParams& params
) {
    double A = params.addLiq.priceDist.amplitudeBrownian;
    std::vector<double> f_density(prices.size(), 0.0);

    for (const Foyer& foyer : foyers_state) {
        double mu = foyer.mu;
        double sigma = foyer.sigma;
        double coeff = A / (std::sqrt(2 * PI) * sigma);

        for (size_t i = 0; i < prices.size(); ++i) {
            double x = prices[i];
            double expo = std::exp(-0.5 * std::pow((x - mu) / sigma, 2));
            f_density[i] += coeff * expo;
        }
    }

    if (foyers_state.size() == 0) {
        return f_density;
    }

    double sum = std::accumulate(f_density.begin(), f_density.end(), 0.0);
    for (double& d : f_density) d /= sum;

    return f_density;
}
