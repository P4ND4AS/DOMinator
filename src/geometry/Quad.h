#pragma once
#include <../include/glad/glad.h>

class Quad {
public:
    Quad();
    void render() const;
    ~Quad();
 
private:
    GLuint VAO, VBO, EBO;
};