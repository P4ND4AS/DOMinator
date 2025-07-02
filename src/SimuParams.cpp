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
			.powDistParam = 0.3,
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
		.powDistParam = 0.3,
		.weight_brownian = 0.5,
		.weight_power = 0.5,
	},

	.marketOrder = {
		.e = 0.9
	}
};


//--------------- BOUNDARIES VALUES --------------

SimuParamBounds gSimuParamBounds = {
	.addLiq = {
		.priceDist = {
			.mu_jitter = {0.0, 0.2},
			.mu_init = {19990.0, 20010.0},
			.p_birth = {0.0, 0.001},
			.p_death = {0.0, 0.001},
			.sigma_jitter = {0.0, 0.2},
			.sigma_init = {0.0, 2.0},
			.amplitudeBrownian = {0.0, 10.0},
			.powDistParam = {0.0, 2.0},
			.weight_brownian = {0.0, 2.0},
			.weight_power = {0.0, 2.0},
		},
	},

	.removeLiq = {
		.mu_jitter = {0.0, 0.2},
		.mu_init = {19990.0, 20010.0},
		.p_birth = {0.0, 0.001},
		.p_death = {0.0, 0.001},
		.sigma_jitter = {0.0, 0.2},
		.sigma_init = {0.0, 2.0},
		.amplitudeBrownian = {0.0, 10.0},
		.powDistParam = {0.0, 2.0},
		.weight_brownian = {0.0, 2.0},
		.weight_power = {0.0, 2.0},
	},

	.marketOrder = {
		.e = {0.0, 2.0}
	}
};