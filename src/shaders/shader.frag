uniform sampler2D heatmap;
in vec2 fragUV;
out vec4 FragColor;

void main() {
    float value = texture(heatmap, fragUV).r;
    FragColor = vec4(vec3(0), 1.0);
}