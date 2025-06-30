#include "../include/text/YAxis.h"
#include "../include/engine/OrderBook.h"
#include <sstream>
#include <iomanip>



void drawYAxis(
    int nLabels,
    int nRows,
    float yAxisX, float yAxisY, float yAxisWidth, float yAxisHeight,
    int windowWidth, int windowHeight,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& textQuad,
    int offset
) {
    // Paramètres axe Y
    float scale = 0.25f;

    // Hauteur d'un label (exemple avec '0')
    float glyphHeightPx = textRenderer.getGlyphHeight('0') * scale;

    for (int i = 0; i < nLabels; ++i) {
        int row = i * (nRows - 1) / (nLabels - 1);
        double price = initialPrice - ((nRows - 1) / 2 - (row + offset)) * ticksize;

        float y_ndc = (float(row) / float(nRows - 1));
        float y_px = yAxisY + y_ndc * yAxisHeight - i * glyphHeightPx / nLabels;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << price;
        std::string label = oss.str();

        // Centrage du texte dans la largeur de l'axe Y
        float labelWidthPx = textRenderer.getGlyphHeight('0')*scale;
        float labelX = yAxisX + (yAxisWidth - labelWidthPx) * 0.5f;


        // Affichage du texte, centré dans le quad axe Y
        textRenderer.drawText(
            textShader,
            label,
            yAxisX, 
            y_px,
            scale,
            textQuad,
            windowWidth, windowHeight,
            glm::vec3(0.8f, 0.8f, 0.8f)
        );
    }
}

