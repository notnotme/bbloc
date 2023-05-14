#include "Shader.h"

#include <stdexcept>

GLuint compileShader(GLenum type, std::string_view src) {
    GLint success;
    auto id = glCreateShader(type);

    auto cString = src.data();
    auto cStringLen = (GLint) src.length();
    glShaderSource(id, 1, &cString, &cStringLen);
    glCompileShader(id);

    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar buf[512] = { 0 };
        glGetShaderInfoLog(id, 512, nullptr, buf);
        throw std::runtime_error(std::string(buf));
    }

    return id;
}

void checkProgram(GLuint id) {
    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar buf[512] = { 0 };
        glGetProgramInfoLog(id, 512, nullptr, buf);
        throw std::runtime_error(std::string(buf));
    }
}
