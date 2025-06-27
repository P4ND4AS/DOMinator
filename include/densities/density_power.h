#pragma once
#include <iostream>
#include <vector>
#include "engine/SimuParams.h"
#include "engine/Order.h"




std::vector<double> density_power(
	const Side& side,
	const double currentBestBid,
	const double currentBestAsk,
	const std::vector<double>& prices,
	const double& powDistParam

);


std::vector<double> density_power_globale(
	const double currentBestBid,
	const double currentBestAsk,
	const std::vector<double>& prices,
	const double& powDistParam

);