#include "engine/OrderBook.h"
#include "../include/Shader.h"
#include "geometry/Quad.h"
#include "../include/text/TextRenderer.h"
#include "../include/input_callbacks.h"
#include "../include/text/YAxis.h"
#include <iostream>
#include <string> 
#include <windows.h>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "Heatmap.h"
#include <ft2build.h>
#include <random>


#include FT_FREETYPE_H

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

std::random_device rd;
std::mt19937 rng(rd());    // Générateur unique


const char* fontPath = "fonts/RobotoMono-Regular.ttf";

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    std::cout << "Création OrderBook..." << std::endl;
    OrderBook ob;
    std::cout << "Ajout liquidité initiale..." << std::endl;
    ob.setInitialLiquidity(500, rng);

    const int n_iter = 10000;
    std::cout << "Début simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    //ob.update(n_iter);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_micro = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double duration_sec = duration_micro / 1e6;

    std::cout << "\nSimulation terminée en " << duration_sec << " secondes (" << duration_micro << " µs)\n";


    // 1. Initialisation GLFW
    if (!glfwInit()) {
        std::cerr << "Échec de l'initialisation de GLFW" << std::endl;
        return -1;
    }
    // Configuration GLFW (OpenGL 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Création de la fenêtre
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bookmap Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "Échec de la création de la fenêtre GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3. Initialisation GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Échec de l'initialisation de GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Key handling
    glfwSetKeyCallback(window, key_callback);


    Quad quad;
    TextRenderer textRenderer(fontPath, 48);

    // Heatmap
    int nRows = 121;
    int nCols = SCR_WIDTH * 0.8f;
    Shader heatmapShader("src/shaders/heatmap.vert", "src/shaders/heatmap.frag");
    Heatmap heatmap(nRows, nCols);
    glm::mat4 heatmapModel = glm::mat4(1.0f);
    heatmapModel = glm::scale(heatmapModel, glm::vec3(0.8f, 0.8f, 1.0f));

    // Text
    Shader textShader("src/shaders/text.vert", "src/shaders/text.frag");


    int iter = 1;
    while (!glfwWindowShouldClose(window)) {
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        BookSnapshot snapshot = ob.getCurrentBook();
        double last_price = snapshot.last_price;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << last_price;
        std::string last_price_str = oss.str();

        // --- Simulation : update uniquement si pas en pause ---
        if (!isPaused) {
            heatmap.update(snapshot);

            if (iter % 3 == 0) {
                ob.update(12000, rng);
            }
            iter++;
            
        }

        // --- Rendu graphique (toujours affiché, même en pause) ---
        glClearColor(0.2f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        heatmap.render(heatmapShader, quad, heatmapModel);

        // Affichage du prix
        textRenderer.drawText(
            textShader,
            last_price_str,
            0.5f * windowWidth, 0.92f * windowHeight,
            0.3f,
            quad,
            windowWidth, windowHeight,
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        // Paramètres heatmap
        float heatmapX = 0.1f * windowWidth;
        float heatmapY = 0.1f * windowHeight;
        float heatmapWidth = 0.8f * windowWidth;
        float heatmapHeight = 0.8f * windowHeight;

        //Affichage de l'axe Y
        drawYAxis(
            10,
            nRows,
            heatmapX,   
            heatmapY,                  
            heatmapWidth,                        
            heatmapHeight,             
            windowWidth, windowHeight,
            textRenderer,
            textShader,
            quad
        );

        // --- Overlay "PAUSE" si besoin ---
        if (isPaused) {
            textRenderer.drawText(
                textShader,
                "PAUSE",
                0.5f * windowWidth, 0.04f * windowHeight,
                0.5f,
                quad,
                windowWidth, windowHeight,
                glm::vec3(1.0f, 0.3f, 0.3f)
            );
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    quad.~Quad();
    heatmapShader.~Shader();
    heatmap.~Heatmap();

    textShader.~Shader();
    textRenderer.~TextRenderer();
    glfwTerminate();
    return 0;
}