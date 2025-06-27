#include "engine/SimuParams.h"

SimuParams gSimuParams = {

	.addLiq = {
		.priceDist = {
			.mu_jitter = 0.05,
			.mu_init = 20010.00,
			.p_birth = 0.0001,
			.p_death = 0.0001,
			.sigma_jitter = 0.1,   
			.sigma_init = 0.5,
			.amplitudeBrownian = 1,
			.powDistParam = 0.5,
			.weight_brownian = 0.5,
			.weight_power = 0.5,
		},
	},

	.removeLiq = {
		.mu_jitter = 0.05,
		.mu_init = 20010.00,
		.p_birth = 0.0001,
		.p_death = 0.0001,
		.sigma_jitter = 0.1,
		.sigma_init = 0.5,
		.amplitudeBrownian = 1,
		.powDistParam = 0.5,
		.weight_brownian = 0.5,
		.weight_power = 0.5,
	},

	.marketOrder = {
		.e = 0.9
	}
};