#version 330 core
out vec4 FragColor;

uniform vec3 quadColor;

void main() {
    FragColor = vec4(quadColor, 1.0);
}