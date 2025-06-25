#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;    // Texture du glyph
uniform vec3 textColor;    // Couleur du texte

void main()
{
    float alpha = texture(text, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}