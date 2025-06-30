#include "renderDomHistogram.h"

void renderDomHistogram(
    const std::vector<float>& normalizedDom,
    float startX, float startY,
    float cellHeight, float maxDomWidth,
    const glm::mat4& projection,
    Shader& domShader,
    Quad& quad
) {
    domShader.use();
    domShader.setMat4("projection", projection);

    for (int i = 0; i < normalizedDom.size(); ++i) {
        float value = normalizedDom[i];
        if (value == 0.0f) continue;

        float width = std::abs(value) * maxDomWidth;
        float xpos = startX + maxDomWidth - width;
        float ypos = startY + i * cellHeight;
        glm::vec3 color = (value > 0) ? glm::vec3(0.0f, 0.4f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

        domShader.setVec3("color", color);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(xpos + width / 2.0f, ypos + cellHeight / 2.0f, 0.0f));
        model = glm::scale(model, glm::vec3(width / 2.0f, cellHeight / 2.0f, 1.0f));
        domShader.setMat4("model", model);

        quad.render(domShader, model);
    }

}