#include "Heatmap.h"
#include "OrderBook.h"
#include <iostream>

Heatmap::Heatmap(int r, int c) 
	: rows(r), cols(c), data(r, std::vector<float>(c, 0.0f)) {
	createTexture();
}

void Heatmap::printHeatMap() const {

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			std::cout << data[i][j] << " ";
		}
		std::cout << "\n";
	}
}

Heatmap::~Heatmap() {
	glDeleteTextures(1, &textureID);
}

void Heatmap::createTexture() {
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, cols, rows, 0, GL_RED, GL_FLOAT, data[0].data());
}

void Heatmap::uploadToTexture() {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RED, GL_FLOAT, data[0].data());
}

void Heatmap::scrollLeft() {
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols - 1; ++c)
			data[r][c] = data[r][c + 1];
}

void Heatmap::fillLastColumn(const BookSnapshot& snapshot) {
	double min_price = 20000.00 - (rows / 2) * 0.25;
	for (int r = 0; r < rows; ++r) {
		double price_level = min_price + r * 0.25;
		float volume = 0.0f;

		auto it = snapshot.prices.find(price_level);
		if (it != snapshot.prices.end()) {
			for (const auto& order : it->second) {
				volume += order.size;
			}
		}
		data[r][cols - 1] = volume;
	}
}

void Heatmap::update(const BookSnapshot& snapshot) {
	scrollLeft();
	fillLastColumn(snapshot);
	uploadToTexture();

	/*for (int r = 0; r < rows; ++r)
		std::cout << data[r][cols - 1] << " ";
	std::cout << "\n\n";*/
}

void Heatmap::render(const Shader& shader, const Quad& quad) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	shader.use();
	shader.setInt("heatmap", 0);
	quad.render();
}