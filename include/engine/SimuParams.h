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