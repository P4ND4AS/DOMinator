#include "OrderBook.h"
#include "include/Shader.h"
#include "src/geometry/Quad.h"
#include <iostream>
#include <string> 
#include <windows.h>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>



int main() {
    //SetConsoleOutputCP(CP_UTF8);
    std::cout << "Création OrderBook..." << std::endl;
    OrderBook ob;
    std::cout << "Ajout liquidité initiale..." << std::endl;
    ob.setInitialLiquidity(500);

    const int n_iter = 100000;
    std::cout << "Début simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    ob.update(n_iter);

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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Bookmap Visualizer", NULL, NULL);
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
    glViewport(0, 0, 800, 600);


    Shader shader("src/shaders/shader.vert", "src/shaders/shader.frag");

    Quad renderQuad;


    int rows = 100;
    int cols = 500;
    std::vector<std::vector<float>> heatmap(rows, std::vector<float>(cols, 0.0f));


    GLuint heatmapTex;
    glGenTextures(1, &heatmapTex);
    glBindTexture(GL_TEXTURE_2D, heatmapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, cols, rows, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    while (!glfwWindowShouldClose(window)) {
        ob.update(1);
        ob.createHeatMap(ob.getCurrentBook(), heatmap, rows, cols);

        std::vector<float> flatHeatmap;
        flatHeatmap.reserve(rows * cols);
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                flatHeatmap.push_back(heatmap[r][c]);

        // Nettoyage de l'écran
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        // Met à jour la texture heatmap
        glBindTexture(GL_TEXTURE_2D, heatmapTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RED, GL_FLOAT, flatHeatmap.data());

        // Active le shader et la texture pour le rendu
        shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heatmapTex);
        shader.setInt("heatmap", 0);

        glClear(GL_COLOR_BUFFER_BIT);
        renderQuad.render();

        // Échange les buffers et gère les événements
        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    renderQuad.~Quad();
    shader.~Shader();
    glfwTerminate();
    return 0;


}