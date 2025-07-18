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

#include <cuda_runtime.h>
#include "AI/NeuralNetwork.h"
#include "AI/TradingAI.h"
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
    
    try {
        HMODULE torchCudaDll = LoadLibraryA("torch_cuda.dll");
        if (torchCudaDll == NULL) {
            std::cerr << "Échec du chargement de torch_cuda.dll. Code d'erreur : " << GetLastError() << std::endl;
            return -1;
        }
        if (!torch::cuda::is_available()) {
            std::cerr << "Erreur CUDA : " << std::endl;
            return -1;
        }
        // Définir le device (GPU)
        torch::Device device(torch::kCUDA);

        // Créer un MemoryBuffer avec T_max = 10
        int64_t T_max = 10;
        MemoryBuffer buffer(T_max);

        // Générer 10 transitions fictives
        for (int i = 0; i < T_max; ++i) {
            // Créer une heatmap fictive (1, 401, 800)
            torch::Tensor heatmap = torch::rand({ 1, 401, 800 }).to(device);
 

            // État de l'agent (-1, 0, ou 1)
            int agent_position = (i % 3) - 1;
            torch::Tensor agent_state = torch::tensor({ static_cast<float>(agent_position) }).to(device); // 1D [1]

            // Action (0: BUY, 1: SELL, 2: WAIT)
            Action action = static_cast<Action>(i % 3);
            torch::Tensor action_tensor = torch::tensor({ static_cast<int64_t>(action) }).to(device); // 1D [1]

            // Log-probabilité fictive
            torch::Tensor log_prob = torch::tensor({ -std::log(3.0f) + 0.1f * i }).to(device); // 1D [1]

            // Récompense fictive
            torch::Tensor reward = torch::tensor({ 0.1f * (i + 1) }).to(device); // 1D [1]

            // Valeur fictive
            torch::Tensor value = torch::tensor({ 0.5f + 0.05f * i }).to(device); // 1D [1]

            // Done (0 sauf pour la dernière transition)
            torch::Tensor done = torch::tensor({ i == T_max - 1 ? 1.0f : 0.0f }).to(device); // 1D [1]

            // Vérifier les tenseurs avant stockage
            std::cout << "Transition " << i << ": "
                << "heatmap_shape=" << heatmap.sizes()
                << "reward_shape=" << reward.sizes()
                << ", reward_device=" << (reward.is_cuda() ? "CUDA" : "CPU")
                << ", done_shape=" << done.sizes()
                << ", done_device=" << (done.is_cuda() ? "CUDA" : "CPU") << std::endl;

            // Créer et stocker la transition
            Transition transition{ heatmap, agent_state, action_tensor, log_prob, reward, value, done };
            buffer.store(transition);

            std::cout << "Stored transition " << i << ": action=" << action
                << ", reward=" << reward.item<float>()
                << ", value=" << value.item<float>()
                << ", done=" << done.item<float>()
                << ", device=" << (reward.is_cuda() ? "CUDA" : "CPU") << std::endl;

            torch::cuda::synchronize();
        }

        // Tester get()
        std::cout << "\n=== Testing get() ===\n";
        auto [heatmaps, agent_states, actions, log_probs, rewards, values, dones] = buffer.get();
        std::cout << "Heatmaps shape: " << heatmaps.sizes() << ", device: " << (heatmaps.is_cuda() ? "CUDA" : "CPU") << std::endl;
        std::cout << "Agent states shape: " << agent_states.sizes() << std::endl;
        std::cout << "Actions shape: " << actions.sizes() << std::endl;
        std::cout << "Rewards: " << rewards << std::endl;
        torch::cuda::synchronize();

        // Tester computeReturns
        std::cout << "\n=== Testing computeReturns ===\n";
        float last_value = 0.6f;
        torch::Tensor returns = buffer.computeReturns(last_value, 0.99f);
        std::cout << "Returns shape: " << returns.sizes() << ", values: " << returns << std::endl;
        torch::cuda::synchronize();

        // Tester computeAdvantages
        std::cout << "\n=== Testing computeAdvantages ===\n";
        torch::Tensor advantages = buffer.computeAdvantages(last_value, 0.99f, 0.95f);
        std::cout << "Advantages shape: " << advantages.sizes() << ", values: " << advantages << std::endl;
        torch::cuda::synchronize();

        // Tester sampleMiniBatch
        std::cout << "\n=== Testing sampleMiniBatch ===\n";
        auto [batch_heatmaps, batch_states, batch_actions, batch_log_probs, batch_rewards, batch_values, batch_dones] =
            buffer.sampleMiniBatch(5, rng);
        std::cout << "Mini-batch heatmaps shape: " << batch_heatmaps.sizes() << std::endl;
        std::cout << "Mini-batch actions: " << batch_actions << std::endl;
        std::cout << "Mini-batch rewards: " << batch_rewards << std::endl;
        torch::cuda::synchronize();

        // Vider le buffer
        buffer.clear();
        std::cout << "\nBuffer cleared. Size: " << std::get<0>(buffer.get()).size(0) << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Erreur standard : " << e.what() << std::endl;
        return -1;
    }





    // ----------------------------------- BOOKMAP -----------------------------------


    /*int iter = 1;
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
    ImGui::DestroyContext();*/

    return 0;
}