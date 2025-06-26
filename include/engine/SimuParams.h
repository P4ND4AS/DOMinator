#pragma once

struct PriceDistParams {
    double mu_delta;
    double p_birth;
    double p_death;
    double s_delta;  // <-- Doit bien s'appeler s_delta
    double s_init;
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