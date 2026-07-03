/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
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
        glDeleteShader(id);
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
