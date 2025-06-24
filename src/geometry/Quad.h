#pragma once
#include <../include/glad/glad.h>
#include <glm/glm.hpp>
#include <Shader.h>

class Quad {
public:
    Quad();

    void render(const Shader& shader, const glm::mat4& model) const;
    ~Quad();
 
private:
    GLuint VAO, VBO, EBO;
};