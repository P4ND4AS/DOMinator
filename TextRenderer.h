#pragma once
#include <map>
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "include/Shader.h"



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

	void drawText(Shader& shader, const std::string& text, float x, float y, float scale);

private:
	std::map<char, GlyphTexture> glyphs_;
	FT_Library ft_;
	FT_Face face_;
};