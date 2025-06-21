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

// Dans main(), avant la boucle de rendu
const int matrixSize = 10;
// Fonction pour g�n�rer des donn�es al�atoires (simule un march�)
void generateRandomHeatmap(float data[matrixSize][matrixSize]) {
    for (int i = 0; i < matrixSize; ++i) {
        for (int j = 0; j < matrixSize; ++j) {
            // Simule des clusters (comme des ordres group�s)
            data[i][j] = static_cast<float>(rand()) / RAND_MAX;

            // Ajoute un peu de bruit (spread)
            if (i > 0 && j > 0) {
                data[i][j] = 0.5f * (data[i - 1][j] + data[i][j - 1]) + 0.2f * (rand() / RAND_MAX);
            }
        }
    }
}

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    /*std::cout << "Cr�ation OrderBook..." << std::endl;
    OrderBook ob;
    std::cout << "Ajout liquidit� initiale..." << std::endl;
    ob.setInitialLiquidity(500);

    const int n_iter = 100000;
    std::cout << "D�but simulation..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    ob.update(n_iter);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_micro = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double duration_sec = duration_micro / 1e6;

    std::cout << "\nSimulation termin�e en " << duration_sec << " secondes (" << duration_micro << " �s)\n";

    system("pause");
    return 0;*/


    // 1. Initialisation GLFW
    if (!glfwInit()) {
        std::cerr << "�chec de l'initialisation de GLFW" << std::endl;
        return -1;
    }
    // Configuration GLFW (OpenGL 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Cr�ation de la fen�tre
    GLFWwindow* window = glfwCreateWindow(800, 600, "Bookmap Visualizer", NULL, NULL);
    if (!window) {
        std::cerr << "�chec de la cr�ation de la fen�tre GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 3. Initialisation GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "�chec de l'initialisation de GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, 800, 600);


    Shader shader("src/shaders/shader.vert", "src/shaders/shader.frag");

    Quad renderQuad;



    float heatmapData[matrixSize][matrixSize] = { 0 };

    while (!glfwWindowShouldClose(window)) {
        generateRandomHeatmap(heatmapData);
        // Nettoyage de l'�cran
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        // Active le programme shader
        shader.use();
        shader.setFloatArray("heatmap", &heatmapData[0][0], matrixSize * matrixSize);
        shader.setInt("size", matrixSize);

        glClear(GL_COLOR_BUFFER_BIT);
        renderQuad.render();
        // �change les buffers et g�re les �v�nements
        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 10 FPS
    }

    renderQuad.~Quad();
    shader.~Shader();
    glfwTerminate();
    return 0;


}