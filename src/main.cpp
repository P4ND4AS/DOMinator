#include "OrderBook.h"
#include "../include/Shader.h"
#include "geometry/Quad.h"
#include "../include/TextRenderer.h"
#include <iostream>
#include <string> 
#include <windows.h>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include "Heatmap.h"
#include <ft2build.h>


#include FT_FREETYPE_H

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}



const char* fontPath = "fonts/RobotoMono-Regular.ttf";

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    std::cout << "Création OrderBook..." << std::endl;
    OrderBook ob;
    std::cout << "Ajout liquidité initiale..." << std::endl;
    ob.setInitialLiquidity(500);

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



    // Heatmap
    Shader heatmapShader("src/shaders/heatmap.vert", "src/shaders/heatmap.frag");
    Quad heatmapQuad;
    Heatmap heatmap(121, 640);
    glm::mat4 heatmapModel = glm::mat4(1.0f);
    heatmapModel = glm::scale(heatmapModel, glm::vec3(0.8f, 0.8f, 1.0f));

    // Text
    Shader textShader("src/shaders/text.vert", "src/shaders/text.frag");
    Quad textQuad;
    TextRenderer textRenderer(fontPath, 48);


    int iter = 1;
    while (!glfwWindowShouldClose(window)) {
        
        BookSnapshot snapshot = ob.getCurrentBook();
        double last_price = snapshot.last_price;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << last_price;
        std::string last_price_str = oss.str();

        heatmap.update(snapshot);

        glClearColor(0.2f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        heatmap.render(heatmapShader, heatmapQuad, heatmapModel);

        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // Exemple pour afficher du texte blanc
        textRenderer.drawText(
            textShader,        // shader texte
            last_price_str,        // texte à afficher
            0.5f * windowWidth, 0.9f * windowHeight,    // position en pixels
            0.3f,              // scale
            textQuad,          // quad unité
            windowWidth, windowHeight,
            glm::vec3(1.0f, 1.0f, 1.0f) // couleur blanche
        );



        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
     
        if (iter % 5 == 0) {
            ob.update(20000);
        }
        iter++;
    }

    heatmapQuad.~Quad();
    heatmapShader.~Shader();
    heatmap.~Heatmap();

    textQuad.~Quad();
    textShader.~Shader();
    textRenderer.~TextRenderer();
    glfwTerminate();
    return 0;
}