#version 330 core
layout (location = 0) in vec2 aPos;    // Position du sommet du quad unité [-1,1]
layout (location = 1) in vec2 aTex;    // Coordonnées texture

uniform mat4 model;
uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoords = vec2(aTex.x, 1.0 - aTex.y);
}