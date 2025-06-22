#include "Heatmap.h"
#include "OrderBook.h"
#include <iostream>
#include <algorithm>

Heatmap::Heatmap(int r, int c, double min_p, double max_p) 
	: rows(r), cols(c), data(r, std::vector<float>(c, 0.0f)),
	  last_price_row_history(c, 0), min_price(min_p), max_price(max_p) 
{
	createTexture();
}

Heatmap::~Heatmap() {
	glDeleteTextures(1, &textureID);
	glDeleteTextures(1, &last_price_textureID);
}


void Heatmap::printHeatMap() const {

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			std::cout << data[i][j] << " ";
		}
		std::cout << "\n";
	}
}


// -------- Création et upload de la texture 2D (heatmap) --------
void Heatmap::createTexture() {
	std::vector<float> linearData;
	linearData.reserve(rows * cols);
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols; ++c)
			linearData.push_back(data[r][c]);

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, cols, rows, 0, GL_RED, GL_FLOAT, linearData.data());
}

void Heatmap::uploadToTexture() {
	std::vector<float> linearData;
	linearData.reserve(rows * cols);
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols; ++c)
			linearData.push_back(data[r][c]);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RED, GL_FLOAT, linearData.data());
}


// -------- Création et upload de la texture 1D (ligne du prix) --------
void Heatmap::createLastPriceTexture() {
	glGenTextures(1, &last_price_textureID);
	glBindTexture(GL_TEXTURE_1D, last_price_textureID);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Initialise avec des zéros (ligne non affichée au début)
	std::vector<float> zeros(cols, 0.0f);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, cols, 0, GL_RED, GL_FLOAT, zeros.data());
}

// Envoie le vecteur des lignes du trait (normalisé [0,1]) à la texture 1D
void Heatmap::uploadLastPriceTexture() {
	std::vector<float> normed(cols, 0.0f);
	for (int c = 0; c < cols; ++c) {
		normed[c] = float(last_price_row_history[c]) / float(rows - 1);
	}
	glBindTexture(GL_TEXTURE_1D, last_price_textureID);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, cols, GL_RED, GL_FLOAT, normed.data());
}

// --------- Logique de la heatmap ---------

void Heatmap::scrollLeft() {
	for (int r = 0; r < rows; ++r)
		for (int c = 0; c < cols - 1; ++c)
			data[r][c] = data[r][c + 1];

	for (int c = 0; c < cols - 1; ++c)
		last_price_row_history[c] = last_price_row_history[c + 1];
}

void Heatmap::fillLastColumn(const BookSnapshot& snapshot) {

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


// Convertit le prix en numéro de ligne
int Heatmap::price_to_row(double price) const {
	double norm = (price - min_price) / (max_price - min_price);
	int row = rows - 1 - static_cast<int>(norm * (rows - 1));
	return std::clamp(row, 0, rows - 1);
}


// --------- Mise à jour de la heatmap et du trait ---------
void Heatmap::update(const BookSnapshot& snapshot) {
	scrollLeft();
	fillLastColumn(snapshot);
	int row = price_to_row(snapshot.last_price);
	last_price_row_history[cols - 1] = row;
	//std::cout << "Ligne du last_price dans la matrice : " << row<<"\n";
	uploadToTexture();
	uploadLastPriceTexture();
}

void Heatmap::render(const Shader& shader, const Quad& quad) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, last_price_textureID);

	shader.use();
	shader.setInt("heatmap", 0);
	shader.setInt("last_price_line", 1);
	shader.setInt("cols", cols);
	shader.setInt("rows", rows);

	quad.render();
}