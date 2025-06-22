#pragma once
#include <vector>
#include <glad/glad.h>
#include "OrderBook.h"
#include "src/geometry/Quad.h"
#include "Shader.h"


class Heatmap {
public:

	int rows, cols;
	std::vector<std::vector<float>> data;

	Heatmap(int r, int c);

	void printHeatMap() const;

	~Heatmap();

	void update(const BookSnapshot& snapshot);
	void render(const Shader& shader, const Quad& quad);

	int getRows() const { return rows; }
	int getCols() const { return cols; }
	GLuint getTexture() const { return textureID;  }

private:

	GLuint textureID;

	void createTexture();
	void uploadToTexture();
	void scrollLeft();
	void fillLastColumn(const BookSnapshot& snapshot);
};