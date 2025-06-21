#include "OrderBook.h"
#include <iostream>
#include <string> 
#include <windows.h>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    /*std::cout << "Création OrderBook..." << std::endl;
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

    system("pause");
    return 0;*/


    // Initialisation de GLFW
    if (!glfwInit()) {
        std::cerr << "Erreur lors de l'initialisation de GLFW" << std::endl;
        return -1;
    }

    // Création de la fenêtre
    GLFWwindow* window = glfwCreateWindow(800, 600, "Fenetre OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erreur lors de la création de la fenêtre" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialisation de GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erreur lors de l'initialisation de GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Boucle d'affichage simple
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;

}