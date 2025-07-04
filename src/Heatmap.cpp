#include "Heatmap.h"
#include "engine/OrderBook.h"
#include <iostream>
#include <algorithm>

Heatmap::Heatmap(int r, int c)
	: view_rows(r), cols(c), data(M, std::vector<float>(c, 0.0f)),
	last_price_row_history(c, 0.0f), domData(M, 0.0f)
{
	createTexture();
	createLastPriceTexture();
}

Heatmap::~Heatmap() {
	glDeleteTextures(1, &textureID);
	glDeleteTextures(1, &last_price_textureID);
}


void Heatmap::printHeatMap() const {

	for (int i = 0; i < M; ++i) {
		for (int j = 0; j < cols; ++j) {
			std::cout << data[i][j] << " ";
		}
		std::cout << "\n";
	}
}


// -------- Cr�ation et upload de la texture 2D (heatmap) --------
void Heatmap::createTexture() {
	std::vector<float> linearData;
	linearData.reserve(M * cols);
	for (int r = (M-view_rows)/2; r < (M+view_rows)/2; ++r)
		for (int c = 0; c < cols; ++c)
			linearData.push_back(data[r+offset][c]);

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, cols, view_rows, 0, GL_RED, GL_FLOAT, linearData.data());
}

void Heatmap::uploadToTexture() {
	std::vector<float> linearData;
	linearData.reserve(M * cols);
	for (int r = (M - view_rows) / 2; r < (M + view_rows) / 2; ++r)
		for (int c = 0; c < cols; ++c)
			linearData.push_back(data[r+offset][c]);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, view_rows, GL_RED, GL_FLOAT, linearData.data());
}


// -------- Cr�ation et upload de la texture 1D (ligne du prix) --------
void Heatmap::createLastPriceTexture() {
	// Suppose: last_price_row_history contient la ligne du trait pour chaque colonne (taille = cols)
	std::vector<float> normed(cols);
	for (int c = 0; c < cols; ++c) {
		normed[c] = float(last_price_row_history[c] - ((M - view_rows) / 2) - offset) / float(view_rows - 1);
		normed[c] = std::clamp(normed[c], 0.0f, 1.0f);
	}

	glGenTextures(1, &last_price_textureID);
	glBindTexture(GL_TEXTURE_1D, last_price_textureID);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, cols, 0, GL_RED, GL_FLOAT, normed.data());
}

void Heatmap::uploadLastPriceTexture() {
	std::vector<float> normed(cols);
	for (int c = 0; c < cols; ++c) {
		normed[c] = float(last_price_row_history[c] - ((M - view_rows) / 2) - offset) / float(view_rows - 1);
		normed[c] = std::clamp(normed[c], 0.0f, 1.0f);
	}

	glBindTexture(GL_TEXTURE_1D, last_price_textureID);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, cols, GL_RED, GL_FLOAT, normed.data());
}

// --------- Logique de la heatmap ---------

void Heatmap::scrollLeft() {
	for (int r = 0; r < M; ++r)
		for (int c = 0; c < cols - 1; ++c)
			data[r][c] = data[r][c + 1];

	for (int c = 0; c < cols - 1; ++c)
		last_price_row_history[c] = last_price_row_history[c + 1];
}

void Heatmap::fillLastColumn(const BookSnapshot& snapshot) {

	for (int r = 0; r < M; ++r) {
		double price_level = min_price + r * ticksize;
		float volume = 0.0f;
		int sign = 0;

		auto it = snapshot.prices.find(price_level);
		if (it != snapshot.prices.end()) {
			for (const auto& order : it->second) {
				volume += order.size;
				sign = (order.side == Side::BID) ? 1 : -1;
			}
		}
		data[r][cols - 1] = volume;
		domData[r] = volume * sign;
	}
}

// Normaliser les donn�es du DOM � l'�cran
std::vector<float> Heatmap::getNormalizedDomData(int offset, int view_rows) const {
	std::vector<float> normalized;
	float max_abs = 0.0f;

	for (int r = (M - view_rows) / 2; r < (M + view_rows) / 2; ++r) {
		max_abs = std::max(max_abs, std::abs(domData[r+offset]));
	}
	if (max_abs == 0.0f) max_abs = 1.0f;

	for (int r = (M - view_rows) / 2; r < (M + view_rows) / 2; ++r) {
		normalized.push_back(domData[r+offset] / max_abs);
	}
	return normalized;
}


// Convertit le prix en num�ro de ligne
int Heatmap::price_to_row(double price) const {
	double norm = (price - min_price) / (max_price - min_price);
	int row = static_cast<int>(norm * (M - 1));
	return std::clamp(row, 0, M - 1);
}


// --------- Mise � jour de la heatmap et du trait ---------
void Heatmap::updateData(const BookSnapshot& snapshot) {
	scrollLeft();
	fillLastColumn(snapshot);
	int row = price_to_row(snapshot.last_price);
	last_price_row_history[cols - 1] = row;
}

void Heatmap::updateTexture() {
	uploadToTexture();
	uploadLastPriceTexture();
}

void Heatmap::render(const Shader& shader, const Quad& quad, const glm::mat4& model,
					 int windowWidth, int windowHeight) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D, last_price_textureID);

	glm::mat4 projection = glm::ortho(
		0.0f, float(windowWidth),
		0.0f, float(windowHeight),
		-1.0f, 1.0f
	);

	shader.use();
	shader.setInt("heatmap", 0);
	shader.setInt("last_price_line", 1);
	shader.setInt("cols", cols);
	shader.setInt("M", M);
	shader.setMat4("projection", projection);

	quad.render(shader, model);
}

void Heatmap::ResampleHeatmapForWindow(int newCols) {
	// Sauvegarde de l'ancienne data
	std::vector<std::vector<float>> oldData = data;
	std::vector<float> oldlast_price_row_history = last_price_row_history;
	int oldCols = cols;

	// Redimensionnement au nouveau format
	data.assign(M, std::vector<float>(newCols, 0.0f)); // M lignes, newCols colonnes, init � 0
	last_price_row_history.assign(newCols, 0.0f);

	int copyCount = std::min(oldCols, newCols);
	int oldStart = oldCols - copyCount;
	int newStart = newCols - copyCount;

	for (int col = 0; col < copyCount; ++col) {
		int oldCol = oldStart + col;
		int newCol = newStart + col;
		for (int row = 0; row < M; ++row) {
			data[row][newCol] = oldData[row][oldCol];
		}
		last_price_row_history[newCol] = oldlast_price_row_history[oldCol];
	}

	cols = newCols;


	if (glIsTexture(textureID)) {
		glDeleteTextures(1, &textureID);
	}
	if (glIsTexture(last_price_textureID)) {
		glDeleteTextures(1, &last_price_textureID);
	}

	createTexture();
	createLastPriceTexture();

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cerr << "OpenGL error after texture creation: " << err << std::endl;
	}
}