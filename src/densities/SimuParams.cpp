#include "densities/SimuParams.h"

SimuParams gSimuParams = {

	.addLiq = {
		.a = 0.04,
		.b = 1.2,
	},

	.removeLiq = {
		.c = 0.3,
		.d = 0.7,
	},

	.marketOrder = {
		.e = 0.9
	}
};