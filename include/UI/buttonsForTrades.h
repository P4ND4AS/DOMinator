#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "../src/geometry/Quad.h"
#include "text/TextRenderer.h"
#include "engine/OrderBook.h"
#include <glm/glm.hpp>


struct TradeButton {
    float x, y;
    float width, height;
    std::string label;

    float r, g, b; // Couleur
    bool isClicked = false;
};


void renderTradeButtons(
    GLFWwindow* window,
    TradeButton button,
    Shader& buttonShader,
    TextRenderer& textRenderer,
    Shader& textShader,
    Quad& quad,
    int windowWidth, int windowHeight,
    glm::mat4 projection
);