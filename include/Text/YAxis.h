#pragma once

#include <string>
#include "TextRenderer.h"

void drawYAxis(
    int nLabels,
    int nRows,
    float yAxisX, float yAxisY, float yAxisWidth, float yAxisHeight,
    int windowWidth, int windowHeight,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& textQuad,
    int offset
);