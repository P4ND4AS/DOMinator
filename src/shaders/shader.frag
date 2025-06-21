#version 330 core
out vec4 FragColor;
uniform float heatmap[100]; // 10x10
uniform int size;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(800.0, 600.0);
    int x = int(uv.x * size);
    int y = int(uv.y * size);
    float value = heatmap[y * size + x];
    FragColor = vec4(value, 0.0, 1.0 - value, 1.0);
}