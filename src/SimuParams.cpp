#include "engine/SimuParams.h"

SimuParams gSimuParams = {

	.addLiq = {
		.priceDist = {
			.mu_delta = 0.001,
			.p_birth = 0.05,
			.p_death = 0.05,
			.s_delta = 0.005,   
			.s_init = 0.01
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