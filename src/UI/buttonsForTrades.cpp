#include "UI/buttonsForTrades.h"
#include <iostream>

// Helper pour savoir si la souris est dans le rectangle
static bool inRect(double x, double y, float rx, float ry, float rw, float rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

// À appeler dans la boucle de rendu principale
void renderTradeButtonsAndHandleClicks(
    GLFWwindow* window,
    Shader& buttonShader,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& quad,
    OrderBook& ob,
    float domX, float domWidth,
    int windowWidth, int windowHeight,
    glm::mat4 projection
) {
    

    float x = domX + domWidth + 15.0f; 
    float y = 0.8f * windowHeight;  
    float w = 90;
    float h = 40;
    float scale = 0.3;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x + w / 2, y + h / 2, 0.0f));
    model = glm::scale(model, glm::vec3(w / 2, h / 2, 1.0f));

    // --- Rendu des quads avec couleurs ---
    buttonShader.use();
    buttonShader.setVec3("quadColor", glm::vec3(0.2f, 0.8f, 0.3f)); // Vert pour Buy
    buttonShader.setMat4("projection", projection);
    quad.render(buttonShader, model);


    // Texte sur les boutons (centré)
    textRenderer.drawText(
        textShader,
        "Buy Market",
        x + (w - textRenderer.getGlyphWidth('B') * scale * 12) / 2.0f,
        y + (h-textRenderer.getGlyphHeight('0') * scale) / 2.0f,
        scale,
        quad,
        windowWidth, windowHeight,
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    // --- Gestion du clic souris ---
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    bool mouseClicked = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    static bool prevMousePressed = false;
    if (mouseClicked && !prevMousePressed) {
        if (inRect(mouseX, windowHeight-mouseY, x, y, w, h)) {
            // BUY MARKET
            MarketOrder order = ob.generateMarketOrder();
            order.side = Side::ASK;
            order.size = 200;
            ob.processMarketOrder(order);
            std::cout << "[TRADE] Achat market size=" << order.size << std::endl;
        }
    }
    prevMousePressed = mouseClicked;
}