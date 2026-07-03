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
#ifndef SHADER_H
#define SHADER_H

#include <string_view>

#include <glad/glad.h>


/**
 * @brief Compiles a GLSL shader of a given type.
 *
 * @param type The type of shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
 * @param src The GLSL source code.
 * @return The compiled shader ID.
 */
GLuint compileShader(GLenum type, std::string_view src);

/**
 * @brief Validates an OpenGL shader program.
 *
 * Checks if the program linked correctly and throws if validation fails.
 * @param id The OpenGL shader program ID.
 */
void checkProgram(GLuint id);


#endif //SHADER_H
