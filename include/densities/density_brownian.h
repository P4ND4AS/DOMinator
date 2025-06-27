#pragma once
#include "engine/SimuParams.h"
#include <random>

struct Foyer {
	double mu;
	double sigma;
};


void updateFoyerState(
	std::vector<Foyer>& foyers_state,
	const std::vector<double>& prices,
	std::mt19937& rng,
	const double& p_death,
	const double& p_birth,
	const double& sigma_init,
	const double& mu_jitter,
	const double& sigma_jitter
);

std::vector<double> density_brownian(
	const std::vector<Foyer>& foyers_state,
	const std::vector<double>& prices,
	const double& amplitudeBrownian
);