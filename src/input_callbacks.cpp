#include "../include/input_callbacks.h"
#include <iostream>
bool isPaused = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) {
            isPaused = !isPaused;
        }
    }
}