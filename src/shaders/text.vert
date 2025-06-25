#version 330 core
layout (location = 0) in vec3 aPos;    // Position du sommet du quad unité [-1,1]
layout (location = 1) in vec2 aTex;    // Coordonnées texture

uniform mat4 model;

out vec2 TexCoords;

void main()
{
    gl_Position = model * vec4(aPos, 1.0);
    TexCoords = vec2(aTex.x, 1.0 - aTex.y);
}