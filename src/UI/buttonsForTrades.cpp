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
    int windowWidth, int windowHeight
) {
    glm::ortho(0, windowWidth, 0, windowHeight, -1, 1);

    float x = 0.8f * windowWidth; // coin haut gauche voulu
    float y = 0.8f * windowHeight;  // coin haut gauche voulu
    float w = 70;
    float h = 40;
    float y_bas = windowHeight - y - h; // conversion haut->bas

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x + w / 2, y_bas + h / 2, 0.0f));
    model = glm::scale(model, glm::vec3(w / 2, h / 2, 1.0f));
    buttonShader.setMat4("model", model);
    quad.render(buttonShader, model);

    // --- Rendu des quads avec couleurs ---
    buttonShader.use();
    buttonShader.setVec3("quadColor", glm::vec3(0.2f, 0.8f, 0.3f)); // Vert pour Buy
    quad.render(buttonShader, model);


    // Texte sur les boutons (centré)
    textRenderer.drawText(
        textShader,
        "Buy Market",
        x + w / 2,
        y_bas + h / 2 - 12, // ajuste -12 si besoin pour centrer verticalement
        0.35f,
        quad,
        windowWidth, windowHeight,
        glm::vec3(1, 1, 1)
    );

    // --- Gestion du clic souris ---
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    bool mouseClicked = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    static bool prevMousePressed = false;

    if (mouseClicked && !prevMousePressed) {
        if (inRect(mouseX, mouseY, x, y, w, h)) {
            // ACHAT MARKET
            MarketOrder order = ob.generateMarketOrder();
            order.side = Side::BID;  // Force achat
            ob.processMarketOrder(order);
            std::cout << "[TRADE] Achat market size=" << order.size << std::endl;
        }
    }
    prevMousePressed = mouseClicked;
}