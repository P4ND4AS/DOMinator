#pragma once

struct PriceDistParams {
    double mu_jitter;
    double mu_init;
    double p_birth;
    double p_death;
    double sigma_jitter;  // <-- Doit bien s'appeler s_delta
    double sigma_init;
    double amplitudeBrownian;
    double powDistParam;
    double weight_brownian;
    double weight_power;
    double weight_uniform;
};

struct AddLiqParams {
    PriceDistParams priceDist;
    double a;
};

struct RemoveLiqParams {
    double c;
    double d;
    // autres param�tres
};

struct MarketOrderParams {
    double e;
    // autres param�tres
};

struct SimuParams {
    AddLiqParams addLiq;
    RemoveLiqParams removeLiq;
    MarketOrderParams marketOrder;
    // ... autres groupes �ventuels
};

extern SimuParams gSimuParams;