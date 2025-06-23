#version 330 core

uniform sampler2D heatmap;
uniform sampler1D last_price_line;
uniform int cols;
uniform int rows; 

in vec2 fragUV;
out vec4 FragColor;

// Palette de 12 couleurs (définies en float 0..1)
vec3 heatPalette(float x) {
    const vec3 colors[12] = vec3[](
        vec3(0.094, 0.223, 0.321),  // #183952
        vec3(0.149, 0.372, 0.529),  // #265f87
        vec3(0.286, 0.514, 0.678),  // #4983ad
        vec3(0.427, 0.678, 0.839),  // #6daad6
        vec3(0.675, 0.811, 0.910),  // #accfe8
        vec3(0.906, 0.910, 0.675),  // #e7e8ac
        vec3(0.890, 0.839, 0.373),  // #e3d65f
        vec3(0.890, 0.710, 0.373),  // #e3b55f
        vec3(0.871, 0.522, 0.251),  // #de8540
        vec3(0.871, 0.384, 0.251),  // #de6240
        vec3(0.871, 0.251, 0.251),  // #de4040
        vec3(0.569, 0.110, 0.110)   // #911c1c
    );
    x = clamp(x, 0.0, 1.0);
    float idx = x * 11.0; // 11 = nombre de couleurs - 1
    int idx0 = int(floor(idx));
    int idx1 = min(idx0 + 1, 11);
    float t = idx - float(idx0);
    return mix(colors[idx0], colors[idx1], t);
}

void main() {
    vec2 uv = fragUV;

    int col = int(fragUV.x * float(cols));
    col = clamp(col, 0, cols - 1);

    float trait_y = texture(last_price_line, float(col) / float(cols)).r;
    float dist = abs(uv.y - trait_y);

    if (dist < 0.5 / float(300)) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    } else {
        float value = texture(heatmap, uv).r;
        float norm = clamp(value / 100 , 0.0, 1.0);
        norm = pow(norm, 1.0); 

        FragColor = vec4(heatPalette(norm), 1.0);
    }
}