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
	const SimuParams& params
);

std::vector<double> density_brownian(
	const std::vector<Foyer>& foyers_state,
	const std::vector<double>& prices,
	const SimuParams& params
);