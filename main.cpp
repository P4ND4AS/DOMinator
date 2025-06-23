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
#include "Heatmap.h"


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

    Heatmap heatmap(121, 640, 19985.25, 20015.00);

    int iter = 1;
    while (!glfwWindowShouldClose(window)) {
        
        BookSnapshot snapshot = ob.getCurrentBook();
        heatmap.update(snapshot);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        heatmap.render(shader, renderQuad);

        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
     
        if (iter % 5 == 0) {
            ob.update(20000);
        }
        iter++;
    }

    renderQuad.~Quad();
    shader.~Shader();
    heatmap.~Heatmap();
    glfwTerminate();
    return 0;





}