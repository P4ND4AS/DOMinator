#pragma once
#include "engine/SimuParams.h"


// --- Descripteur de paramètre pour factorisation UI ---
struct DoubleParamDesc {
    const char* label;
    double* value;
    const DoubleBounds* bounds;
    const char* format = "%.6f";
};

void DrawModelParametersUI(SimuParams& params, const SimuParamBounds& bounds);