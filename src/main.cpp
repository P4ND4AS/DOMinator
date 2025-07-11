#include "engine/OrderBook.h"
#include "../include/Shader.h"
#include "geometry/Quad.h"
#include "../include/text/TextRenderer.h"
#include "../include/input_callbacks.h"
#include "../include/text/YAxis.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "UI/model_parameters.h"
#include "renderDomHistogram.h"
#include "UI/buttonsForTrades.h"
#include "AI/NeuralNetwork.h"
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
#include <iomanip>
#include <sstream>
#include <algorithm>

#include FT_FREETYPE_H

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    Heatmap* heatmap = static_cast<Heatmap*>(glfwGetWindowUserPointer(window));
    if (heatmap) {
        heatmap->ResampleHeatmapForWindow(static_cast<int>(0.6f*width));
    }
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

    // Key and mouse handling
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialisation de ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    Quad quad;
    TextRenderer textRenderer(fontPath, 48);

    // Heatmap
    int viewRows = 121;
    int nCols = static_cast<int>(SCR_WIDTH * 0.6f);
    Shader heatmapShader("src/shaders/heatmap.vert", "src/shaders/heatmap.frag");
    Heatmap heatmap(viewRows, nCols);

    glfwSetWindowUserPointer(window, &heatmap);

    // Text
    Shader textShader("src/shaders/text.vert", "src/shaders/text.frag");

    // DOM
    Shader domShader("src/shaders/domBar.vert", "src/shaders/domBar.frag");

    // Button to take a trade
    Shader buttonShader("src/shaders/button.vert", "src/shaders/button.frag");


    // ------------------- AI SETUP -------------------
    AIConfig config = loadConfig("src/AI/configAI.json");    
  
    Eigen::MatrixXf M = Eigen::MatrixXf::Random(config.inputHeight, config.inputWidth);

    PolicyValueNet net(config);

    auto [policy, value] = net.forward(M);
 
    std::cout << "(policy head):\n" << policy.transpose() << "\n";
    std::cout << "(value head): " << value << "\n";
    // ------------------------------------------------




    int iter = 1;
    while (!glfwWindowShouldClose(window)) {
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        // --- Démarre une nouvelle frame ImGui ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        DrawModelParametersUI(gSimuParams, gSimuParamBounds);

        BookSnapshot snapshot = ob.getCurrentBook();
        double last_price = snapshot.last_price;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << last_price;
        std::string last_price_str = oss.str();

        // --- Simulation : update uniquement si pas en pause ---
        if (!isPaused) {
            heatmap.updateData(snapshot);

            if (iter % 3 == 0) {
                ob.update(22000, rng);

            }
            iter++;

        }

       

        // --- Rendu graphique (toujours affiché, même en pause) ---
        glClearColor(0.2f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Paramètres d'affichage pour bien organiser chaque élément à l'écran ---
        float paddingDOM = 0.005f * windowWidth;
        float yAxisWidth = (70.0f < 0.09f * windowWidth) ? 70.0f : 0.09f * windowWidth;
        float domWidth = (80.0f < 0.1f * windowWidth) ? 80.0f : 0.1f * windowWidth;

        float heatmapX = 0.005f * windowWidth;
        float heatmapY = 0.1f * windowHeight;
        float heatmapWidth = windowWidth * 0.6f;
        float heatmapHeight = 0.8f * windowHeight;

        float yAxisX = heatmapX + heatmapWidth + paddingDOM;
        float domX = yAxisX + yAxisWidth + paddingDOM;

        // Trading Buttons
        float buyMarketX = domX + domWidth + 0.02f * windowWidth;
        float buyMarketY = 0.8f * windowHeight;
        float buyMarketWidth = (80.0f < 0.1f * windowWidth) ? 80.0f : 0.1f * windowWidth;
        float buyMarketHeight = 40.0f;
        float paddingButtons = 8.0f;

        glm::mat4 heatmapModel = glm::mat4(1.0f);
        // Translation : place le centre du quad à (heatmapX + heatmapWidth/2, heatmapY + heatmapHeight/2)
        heatmapModel = glm::translate(heatmapModel, glm::vec3(
            heatmapX + heatmapWidth / 2.0f,
            heatmapY + heatmapHeight / 2.0f,
            0.0f
        ));
        
        // Scale : adapte la taille du quad à la taille voulue en pixels
        heatmapModel = glm::scale(heatmapModel, glm::vec3(
            heatmapWidth / 2.0f,
            heatmapHeight / 2.0f,
            1.0f
        ));

        heatmap.updateTexture();
        heatmap.render(heatmapShader, quad, heatmapModel, windowWidth, windowHeight);

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



        //Affichage de l'axe Y
        drawYAxis(
            10,
            viewRows,
            yAxisX,
            heatmapY,
            yAxisWidth,
            heatmapHeight,
            windowWidth, windowHeight,
            textRenderer,
            textShader,
            quad,
            heatmap.offset
        );

        // DOM
        glm::mat4 projection = glm::ortho(
            0.0f, float(windowWidth),
            0.0f, float(windowHeight),
            -1.0f, 1.0f
        );

        std::vector<float> normalizedDom = heatmap.getNormalizedDomData(heatmap.offset, viewRows);
        renderDomHistogram(normalizedDom, domX, heatmapY, heatmapHeight / viewRows, domWidth, projection,
            domShader, quad);

        std::vector<TradeButton> tradeButtons = {
            {buyMarketX, buyMarketY, buyMarketWidth, buyMarketHeight, "Buy Market", 0.0f, 0.7f, 0.1f},
            {buyMarketX + buyMarketWidth + paddingButtons, buyMarketY, buyMarketWidth, buyMarketHeight, "Sell Market", 0.7f, 0.0f, 0.1f},
            {buyMarketX, buyMarketY + buyMarketHeight + paddingButtons, 2.0f* buyMarketWidth + paddingButtons, buyMarketHeight, "CANCEL", 0.15f, 0.15f, 0.15f}
        };

 

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

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}