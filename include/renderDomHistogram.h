#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"
#include "../src/geometry/Quad.h"

void renderDomHistogram(
	const std::vector<float>& normalizedDom,
	float startX, float startY,
	float cellHeight,
	float maxDomWidth,
	const glm::mat4& projection,
	Shader& domShader,
	Quad& quad
);



