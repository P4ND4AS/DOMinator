#include "UI/buttonsForTrades.h"
#include <iostream>

// Helper pour savoir si la souris est dans le rectangle
static bool inRect(double x, double y, float rx, float ry, float rw, float rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

// À appeler dans la boucle de rendu principale
void renderTradeButtons(
    GLFWwindow* window,
    TradeButton button,
    Shader& buttonShader,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& quad,
    int windowWidth, int windowHeight,
    glm::mat4 projection
) {
    

    float x = button.x;
    float y = button.y;
    float w = button.width;
    float h = button.height;
    float scaleText = 0.25;
    int textWidth = textRenderer.getTextWidth(button.label, scaleText);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x + w / 2, y + h / 2, 0.0f));
    model = glm::scale(model, glm::vec3(w / 2, h / 2, 1.0f));

    // Quad Rendering
    buttonShader.use();
    buttonShader.setVec3("quadColor", glm::vec3(button.r, button.g, button.b));
    buttonShader.setMat4("projection", projection);
    quad.render(buttonShader, model);


    // Text Rendering
    textRenderer.drawText(
        textShader,
        button.label,
        x + (w - textWidth) / 2.0f,
        y + (h-textRenderer.getGlyphHeight('0') * scaleText) / 2.0f,
        scaleText,
        quad,
        windowWidth, windowHeight,
        glm::vec3(1.0f, 1.0f, 1.0f)
    );
}