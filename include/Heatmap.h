#pragma once
#include <vector>
#include <glad/glad.h>
#include "engine/OrderBook.h"
#include "../src/geometry/Quad.h"
#include "Shader.h"
#include <algorithm>


class Heatmap {
public:

	int view_rows, cols;
	int M = 2 * depth + 1;
	std::vector<std::vector<float>> data;

	//Pour la ligne du trait de 'last_price' par colonne
	std::vector<float> last_price_row_history;
	GLuint last_price_textureID;

	double min_price = initialPrice - ((M - 1) / 2) * ticksize;
	double max_price = initialPrice + ((M - 1) / 2) * ticksize; // Limites de prix à afficher sur l'écran

	Heatmap(int r, int c);

	void printHeatMap() const;

	~Heatmap();

	void updateData(const BookSnapshot& snapshot);
	void updateTexture();
	void render(const Shader& shader, const Quad& quad, const glm::mat4& model,
				int windowWidth, int windowHeight);

	int getRows() const { return M; }
	int getCols() const { return cols; }
	GLuint getTexture() const { return textureID; }

	int price_to_row(double price) const;

	int offset = 0;
	void clampOffset() {
		int minOffset = -(M - view_rows) / 2;
		int maxOffset = (M - view_rows) / 2;
		offset = std::clamp(offset, minOffset, maxOffset);
	}

	// Pour l'histogramme du DOM
	std::vector<float> domData;
	std::vector<float> getNormalizedDomData(int offset, int view_rows) const;

	void ResampleHeatmapForWindow(int newCols);

private:

	GLuint textureID;

	void createTexture();
	void uploadToTexture();
	void createLastPriceTexture();
	void uploadLastPriceTexture();

	void scrollLeft();
	void fillLastColumn(const BookSnapshot& snapshot);
};