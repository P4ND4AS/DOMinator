#pragma once
#include <vector>
#include <glad/glad.h>
#include "OrderBook.h"
#include "../src/geometry/Quad.h"
#include "Shader.h"


class Heatmap {
public:

	int rows, cols;
	std::vector<std::vector<float>> data;

	//Pour la ligne du trait de 'last_price' par colonne
	std::vector<float> last_price_row_history;
	GLuint last_price_textureID;

	double min_price = 20000.00 - (rows / 2) * 0.25;
	double max_price = 20000.00 + (rows / 2) * 0.25; // Limites de prix à afficher sur l'écran

	Heatmap(int r, int c);

	void printHeatMap() const;

	~Heatmap();

	void update(const BookSnapshot& snapshot);
	void render(const Shader& shader, const Quad& quad, const glm::mat4& model);

	int getRows() const { return rows; }
	int getCols() const { return cols; }
	GLuint getTexture() const { return textureID;  }

	int price_to_row(double price) const;

private:

	GLuint textureID;

	void createTexture();
	void uploadToTexture();
	void createLastPriceTexture();
	void uploadLastPriceTexture();

	void scrollLeft();
	void fillLastColumn(const BookSnapshot& snapshot);
};