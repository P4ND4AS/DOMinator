#include <random>
#include "densities/density_brownian.h"
#include "engine/OrderBook.h"
#include <cmath>	
#include <algorithm>
#include <iostream>


void updateFoyerState(
	std::vector<Foyer>& foyers_states,
	const std::vector<double>& prices,
	std::mt19937& rng,
	const SimuParams params
) {
	// Supprimer les foyers morts
	std::bernoulli_distribution death_dist(params.addLiq.priceDist.p_death);
	foyers_states.erase(
		std::remove_if(
			foyers_states.begin(), foyers_states.end(),
			[&](const Foyer&) { return death_dist(rng); }
		),
		foyers_states.end()
	);
}


