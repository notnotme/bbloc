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
#ifndef QUAD_PROGRAM_H
#define QUAD_PROGRAM_H

#include <glad/glad.h>


/**
 * @brief Manages a simple shader program for rendering textured quads.
 *
 * This class encapsulates an OpenGL program and its associated vertex array object,
 * providing methods to bind and configure the rendering pipeline.
 */
class QuadProgram final {
private:
    /** Handle to the vertex array object. */
    GLuint m_vao;

    /** Handle to the compiled OpenGL shader program. */
    GLuint m_program;

    /** Handle to the matrix uniform location used for transformations. */
    GLint m_matrix_uniform;

public:
    /** @brief Deleted copy constructor. */
    QuadProgram(const QuadProgram &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadProgram &operator=(const QuadProgram &) = delete;

    /** @brief Constructs an uninitialized QuadProgram object. */
    explicit QuadProgram();

    /** @brief Creates and compiles the shader program and associated VAO. */
    void create();

    /** @brief Releases the OpenGL program and VAO resources. */
    void destroy();

    /** @brief Sets this program as the current one in the OpenGL pipeline. */
    void use() const;

    /**
     * @brief Binds a vertex buffer to the shader's attribute layout.
     *
     * @param buffer OpenGL buffer object to bind.
     */
    void bindVertexBuffer(GLuint buffer) const;

    /**
     * @brief Uploads a 4x4 transformation matrix to the shader program.
     *
     * @param matrix Pointer to 16 floats representing the matrix.
     */
    void setMatrix(const float* matrix) const;

    /**
     * @brief Issues a draw call to render a range of quads from the vertex buffer.
     *
     * @param start First index of the vertex buffer.
     * @param count Number of quads to render.
     */
    void draw(uint32_t start, uint32_t count) const;
};


#endif //QUAD_PROGRAM_H
