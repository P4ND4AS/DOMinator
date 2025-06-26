#pragma once
#include <map>
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../include/Shader.h"
#include "../src/geometry/Quad.h"
#include <string>



struct GlyphTexture {
	unsigned int textureID;
	int width, height;
	int bearingX, bearingY;
	int advance;
};


class TextRenderer {
public:
	TextRenderer(const char* fontPath, int pixelSize);
	~TextRenderer();

	void drawText(Shader& shader, const std::string& text, float x, float y, 
		float scale, Quad& quad, int windowWidth, int windowHeight, const glm::vec3& color);
	
	int getGlyphHeight(char c) const;

private:
	std::map<char, GlyphTexture> glyphs_;
	FT_Library ft_;
	FT_Face face_;
};