#pragma once
#include <vector>
#include "engine/SimuParams.h"

std::vector<double> combine_and_normalize_densities(
	const std::vector<std::vector<double>>& densities,
	const std::vector<double>& weights = {}
);