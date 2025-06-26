#pragma once

struct AddLiqParams {
    double a;
    double b;
    // autres paramètres spécifiques à l'ajout de liquidité
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