#pragma once

#include <string>
#include "TextRenderer.h"

void drawYAxis(
    int nLabels,
    int nRows,
    float heatmapX, float heatmapY, float heatmapWidth, float heatmapHeight,
    int windowWidth, int windowHeight,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& textQuad
);