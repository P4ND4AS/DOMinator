#version 330 core

uniform sampler2D heatmap;
uniform sampler1D last_price_line;
uniform int cols;
uniform int M; 

in vec2 fragUV;
out vec4 FragColor;

// Palette de 13 couleurs (définies en float 0..1)
vec3 heatPalette(float x) {
    const vec3 colors[13] = vec3[](
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
        vec3(0.569, 0.110, 0.110),  // #911c1c
        vec3(0.341, 0.039, 0.039)
    );
    x = clamp(x, 0.0, 1.0);
    float idx = x * 12.0; // 12 = nombre de couleurs - 1
    int idx0 = int(floor(idx));
    int idx1 = min(idx0 + 1, 12);
    float t = idx - float(idx0);
    return mix(colors[idx0], colors[idx1], t);
}

void main() {
    vec2 uv = fragUV;

    int col = int(uv.x * float(cols));
    col = clamp(col, 1, cols-2);
    float rowA = texture(last_price_line, float(col) / float(cols-1)).r;
    float rowB = texture(last_price_line, float(col-1) / float(cols-1)).r;

    float minRow = min(rowA, rowB);
    float maxRow = max(rowA, rowB);
    float eps = 1/float(600); 

    float trait_y = texture(last_price_line, float(col) / float(cols-1)).r;
    float dist = abs(uv.y - trait_y);

    if ((uv.y >= minRow - eps && uv.y <= maxRow + eps && minRow != maxRow) || dist < eps) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        float value = texture(heatmap, uv).r;
        float norm = clamp(value / 200 , 0.0, 1.0);
        norm = pow(norm, 1.0); 
    
        FragColor = vec4(heatPalette(norm), 1.0);
    }
}