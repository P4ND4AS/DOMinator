#include "../include/input_callbacks.h"
#include "Heatmap.h"
#include <iostream>
bool isPaused = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) {
            isPaused = !isPaused;
        }

        //Décalge de la heatmap
        if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
            //Récupérer le pointeur de la heatmap
            Heatmap* heatmap = static_cast<Heatmap*>(glfwGetWindowUserPointer(window));
            if (heatmap) {
                int delta = (key == GLFW_KEY_UP) ? 5 : -5;
                heatmap->offset += delta;        
                heatmap->clampOffset();
            }
        }
    }

}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Heatmap* heatmap = static_cast<Heatmap*>(glfwGetWindowUserPointer(window));
    if (heatmap) {
        heatmap->offset += static_cast<int>(yoffset) * 5;
        heatmap->clampOffset();
    }
}