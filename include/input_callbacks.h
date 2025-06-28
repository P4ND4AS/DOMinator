#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern bool isPaused;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);