#pragma once

struct AddLiqParams {
    double a;
    double b;
    // autres param�tres sp�cifiques � l'ajout de liquidit�
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