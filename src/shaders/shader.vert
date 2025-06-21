layout (location = 0) in vec3 aPos;      // position vertex
layout (location = 1) in vec2 aUV;       // uv du vertex

out vec2 fragUV; // <-- pour passer à ton fragment shader

void main() {
    gl_Position = vec4(aPos, 1.0);
    fragUV = aUV;  // <-- passe l'UV au fragment shader
}