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
};

struct AddLiqParams {
    PriceDistParams priceDist;
};

struct RemoveLiqParams {
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


// --------- BOUNDARIES FOR THE PARAMETERS ---------

struct DoubleBounds {
    double min, max;
};

struct PriceDistBounds {
    DoubleBounds mu_jitter;
    DoubleBounds mu_init;
    DoubleBounds p_birth;
    DoubleBounds p_death;
    DoubleBounds sigma_jitter;
    DoubleBounds sigma_init;
    DoubleBounds amplitudeBrownian;
    DoubleBounds powDistParam;
    DoubleBounds weight_brownian;
    DoubleBounds weight_power;
};

struct AddLiqBounds {
    PriceDistBounds priceDist;
};

struct RemoveLiqBounds {
    DoubleBounds mu_jitter;
    DoubleBounds mu_init;
    DoubleBounds p_birth;
    DoubleBounds p_death;
    DoubleBounds sigma_jitter;
    DoubleBounds sigma_init;
    DoubleBounds amplitudeBrownian;
    DoubleBounds powDistParam;
    DoubleBounds weight_brownian;
    DoubleBounds weight_power;
};

struct MarketOrderBounds {
    DoubleBounds e;
};

struct SimuParamBounds {
    AddLiqBounds addLiq;
    RemoveLiqBounds removeLiq;
    MarketOrderBounds marketOrder;
};

extern SimuParamBounds gSimuParamBounds;