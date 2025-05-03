#include "Shader.h"

#include <stdexcept>


GLuint compileShader(const GLenum type, const std::string_view src) {
    GLint success;
    const auto id = glCreateShader(type);

    const auto c_string = src.data();
    const auto c_string_length = static_cast<GLint>(src.length());
    glShaderSource(id, 1, &c_string, &c_string_length);
    glCompileShader(id);

    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512] = {};
        glGetShaderInfoLog(id, 512, nullptr, info_log);
        throw std::runtime_error(info_log);
    }

    return id;
}

void checkProgram(const GLuint id) {
    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512] = {};
        glGetProgramInfoLog(id, 512, nullptr, info_log);
        throw std::runtime_error(info_log);
    }
}
