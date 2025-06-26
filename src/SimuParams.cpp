#include "engine/SimuParams.h"

SimuParams gSimuParams = {

	.addLiq = {
		.priceDist = {
			.mu_jitter = 0.01,
			.p_birth = 0.0005,
			.p_death = 0.0005,
			.sigma_jitter = 0.01,   
			.sigma_init = 0.1,
			.amplitudeBrownian = 1,
		},
		.a = 0.05,
	},

	.removeLiq = {
		.c = 0.3,
		.d = 0.7,
	},

	.marketOrder = {
		.e = 0.9
	}
};