#include "../include/TextRenderer.h"
#include <iostream>


TextRenderer::TextRenderer(const char* fontPath, int pixelSize)
	: face_(nullptr), ft_(nullptr)
{
	// 1. Initialisation de FreeType
	if (FT_Init_FreeType(&ft_)) {
		std::cerr << "Erreur : Impossible d'initialiser FreeType" << "\n";
		return;
	}

	// 2. Charger la police de caractère
	if (FT_New_Face(ft_, fontPath, 0, &face_)) {
		std::cerr << "Erreur : Impossible de charger la police" << fontPath << "\n";
		return;
	}

	// 3. Définir la taille des glyphes
	FT_Set_Pixel_Sizes(face_, 0, pixelSize);

	// 4. Désactiver l'alignement de paquet pour les textures (important pour FreeType)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 5. Pour chaque caractère ASCII affichable, charger le glyphe et créer la texture OpenGL
	for (unsigned char c = 0; c < 128; ++c) {
		// Charger le glyphe dans FreeType
		if (FT_Load_Char(face_, c, FT_LOAD_RENDER)) {
			std::cerr << "Erreur : Impossible de charger le glyphe '" << c << "'\n";
			continue;
		}

		// Générer la texture OpenGL pour le bitmap du glyphe
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face_->glyph->bitmap.width,
			face_->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face_->glyph->bitmap.buffer);

		// Paramètres de texture (bordure, interpolation)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Remplir la structure du glyph
		GlyphTexture glyph;
		glyph.textureID = texture;
		glyph.width = face_->glyph->bitmap.width;
		glyph.height = face_->glyph->bitmap.rows;
		glyph.bearingX = face_->glyph->bitmap_left;
		glyph.bearingY = face_->glyph->bitmap_top;
		glyph.advance = face_->glyph->advance.x;

		// Stocker ce glyph dans la map
		glyphs_.insert(std::pair<char, GlyphTexture>(c, glyph));
	}

	// 6. (Bonne pratique) Réinitialiser le binding de la texture
	glBindTexture(GL_TEXTURE_2D, 0);
}

TextRenderer::~TextRenderer() {
	// Libérer toutes les textures GPU des glyphes
	for (auto& pair : glyphs_) {
		glDeleteTextures(1, &pair.second.textureID);
	}
	glyphs_.clear();

	// Libérer FreeType : d'abord le face, puis la bibliothèque
	if (face_) {
		FT_Done_Face(face_);
		face_ = nullptr;
	}
	if (ft_) {
		FT_Done_FreeType(ft_);
		ft_ = nullptr;
	}

}

void TextRenderer::drawText(Shader& shader, const std::string& text, float x, float y, 
	float scale, Quad& quad, int windowWidth, int windowHeight, const glm::vec3& color) {


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shader.use();
	shader.setVec3("textColor", color);

	float xCursor = x;
	for (char c : text) {
		auto it = glyphs_.find(c);
		if (it == glyphs_.end()) continue;

		const GlyphTexture& glyph = it->second;

		// Position et taille du glyph en pixels
		float xpos = xCursor + glyph.bearingX * scale;
		float ypos = y - (glyph.height - glyph.bearingY) * scale;
		float w = glyph.width * scale;
		float h = glyph.height * scale;

		// Conversion pixels -> NDC [-1, 1]
		// L'origine (0, 0) est en bas à gauche dans OpenGL
		float xpos_ndc = 2.0f * xpos / windowWidth - 1.0f;
		float ypos_ndc = 2.0f * ypos / windowHeight - 1.0f;


		float w_ndc = 2.0f * w / windowWidth;
		float h_ndc = 2.0f * h / windowHeight;

		// Matrice modèle pour le quad [-1, 1]
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(xpos_ndc + w_ndc / 2.0f, ypos_ndc + h_ndc / 2.0f, 0.0f));
		model = glm::scale(model, glm::vec3(w_ndc / 2.0f, h_ndc / 2.0f, 1.0f));

		// Bind la texture du glyph
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, glyph.textureID);

		quad.render(shader, model);

		xCursor += (glyph.advance >> 6) * scale;
	}
}

int TextRenderer::getGlyphHeight(char c) const {
	auto it = glyphs_.find(c);
	if (it != glyphs_.end()) return it->second.height;
	return 0;
}