#include "../include/input_callbacks.h"
#include "Heatmap.h"
#include <iostream>
bool isPaused = true;

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
            }
        }
    }

}