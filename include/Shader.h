#pragma once
#include <string>
#include <../include/glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>




class Shader {
    public:
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    void use() const;
    GLuint getID() const { return ID; }

    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloatArray(const std::string &name, const float* values, int count) const {
        glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, values);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const;

    private:
    GLuint ID;

    void checkCompileErrors(GLuint shader, const std::string& type);
};