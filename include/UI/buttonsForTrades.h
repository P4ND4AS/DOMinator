#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "../src/geometry/Quad.h"
#include "text/TextRenderer.h"
#include "engine/OrderBook.h"
#include <glm/glm.hpp>

// À appeler à chaque frame dans la boucle principale
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
);