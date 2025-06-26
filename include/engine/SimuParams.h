#pragma once

struct PriceDistParams {
    double mu_jitter;
    double p_birth;
    double p_death;
    double sigma_jitter;  // <-- Doit bien s'appeler s_delta
    double sigma_init;
    double amplitudeBrownian;
};

struct AddLiqParams {
    PriceDistParams priceDist;
    double a;
};

struct RemoveLiqParams {
    double c;
    double d;
    // autres paramètres
};

struct MarketOrderParams {
    double e;
    // autres paramètres
};

struct SimuParams {
    AddLiqParams addLiq;
    RemoveLiqParams removeLiq;
    MarketOrderParams marketOrder;
    // ... autres groupes éventuels
};

extern SimuParams gSimuParams;