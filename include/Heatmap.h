#pragma once
#include <vector>
#include <glad/glad.h>
#include "engine/OrderBook.h"
#include "../src/geometry/Quad.h"
#include "Shader.h"


class Heatmap {
public:

	int view_rows, cols;
	std::vector<std::vector<float>> data;

	//Pour la ligne du trait de 'last_price' par colonne
	std::vector<float> last_price_row_history;
	GLuint last_price_textureID;

	double min_price = initialPrice - depth * ticksize;
	double max_price = initialPrice + depth * ticksize;
	int M = (max_price - min_price) / ticksize + 1;

	Heatmap(int r, int c);

	void printHeatMap() const;

	~Heatmap();

	void update(const BookSnapshot& snapshot);
	void render(const Shader& shader, const Quad& quad, const glm::mat4& model);

	int getRows() const { return M; }
	int getCols() const { return cols; }
	GLuint getTexture() const { return textureID;  }

	int price_to_row(double price) const;

	int offset = 0; // Pour décaler l'affichage de la heatmap à l'écran
	void scrollUp(int delta = 5);
	void scrollDown(int delta = 5);

private:

	GLuint textureID;

	void createTexture();
	void uploadToTexture();
	void createLastPriceTexture();
	void uploadLastPriceTexture();

	void scrollLeft();
	void fillLastColumn(const BookSnapshot& snapshot);
};