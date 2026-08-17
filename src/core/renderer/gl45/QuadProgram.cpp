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
#include "../QuadProgram.h"

#include <cstddef>
#include <stdexcept>

#include "../Shader.h"
#include "../QuadVertex.h"


static constexpr auto VERTEX_SRC = R"text(
    #version 420 core
    precision lowp float;

    layout (location = 0) in vec2 a_translation;
    layout (location = 1) in vec2 a_size;
    layout (location = 2) in vec2 a_texture;
    layout (location = 3) in vec4 a_tint;
    layout (location = 4) in float a_texture_layer;
    layout (location = 5) in float a_texture_unit;

    uniform mat4 u_matrix;

    out vec4 v_tint;
    out vec2 v_texture;
    flat out int v_texture_layer;
    flat out int v_texture_unit;

    void main() {
        vec2 position;
        vec2 tex_coord;

        switch (gl_VertexID) {
            case 0:
                position = vec2(1.0, 0.0);
                tex_coord = vec2(a_texture.x + a_size.x, a_texture.y);
            break;
            case 1:
                position = vec2(0.0, 0.0);
                tex_coord = vec2(a_texture.x, a_texture.y);
            break;
            case 2:
                position = vec2(1.0, 1.0);
                tex_coord = vec2(a_texture.x + a_size.x, a_texture.y + a_size.y);
            break;
            default:
                position = vec2(0.0, 1.0);
                tex_coord = vec2(a_texture.x, a_texture.y + a_size.y);
            break;
        }

        v_tint = a_tint;
        v_texture = tex_coord / 255;
        v_texture_layer = int(a_texture_layer);
        v_texture_unit = int(a_texture_unit);
        gl_Position = u_matrix * vec4(position * a_size + a_translation, 0.0, 1.0);
    }
)text";

static constexpr auto FRAGMENT_SRC = R"text(
    #version 420 core
    precision lowp float;

    in vec4 v_tint;
    in vec2 v_texture;
    flat in int v_texture_layer;
    flat in int v_texture_unit;

    out vec4 o_color;

    layout (binding = 0) uniform sampler2DArray texture_0;
    layout (binding = 1) uniform sampler2DArray texture_1;

    void main() {
        bool use_texture = v_texture_layer < 255;
        vec4 texel = v_texture_unit == 0
            ? texture(texture_0, vec3(v_texture, v_texture_layer))
            : texture(texture_1, vec3(v_texture, v_texture_layer));
        float alpha = mix(1.0, texel.r, use_texture);
        o_color = vec4(v_tint.rgb, v_tint.a * alpha);
    }
)text";

QuadProgram::QuadProgram()
    : m_vao(0),
      m_program(0),
      m_matrix_uniform(-1) {}

void QuadProgram::create() {
    // Create the fragment and vertex shader
    GLuint fragment_shader = 0;
    GLuint vertex_shader = 0;
    try {
        fragment_shader = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SRC);
        vertex_shader = compileShader(GL_VERTEX_SHADER, VERTEX_SRC);
    } catch (...) {
        if (fragment_shader != 0) {
            glDeleteShader(fragment_shader);
        }

        if (vertex_shader != 0) {
            glDeleteShader(vertex_shader);
        }

        throw;
    }

    m_program = glCreateProgram();
    if (m_program == 0) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        throw std::runtime_error("Failed to create program");
    }

    // Link the shaders to the program
    glAttachShader(m_program, fragment_shader);
    glAttachShader(m_program, vertex_shader);
    glLinkProgram(m_program);

    // Delete the shaders and check the program
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    checkProgram(m_program);

    // Get uniforms
    m_matrix_uniform = glGetUniformLocation(m_program, "u_matrix");

    // Create the vertex array object
    glCreateVertexArrays(1, &m_vao);
    if (m_vao == 0) {
        throw std::runtime_error("Failed to create vertex array");
    }

    // Set attributes
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 2, GL_SHORT, GL_FALSE, offsetof(QuadVertex, translation_x));
    glVertexArrayAttribBinding(m_vao, 0, 0);
    glVertexArrayBindingDivisor(m_vao, 0, 1);

    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 2, GL_UNSIGNED_SHORT, GL_FALSE, offsetof(QuadVertex, width));
    glVertexArrayAttribBinding(m_vao, 1, 0);
    glVertexArrayBindingDivisor(m_vao, 1, 1);

    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(QuadVertex, texture_s));
    glVertexArrayAttribBinding(m_vao, 2, 0);
    glVertexArrayBindingDivisor(m_vao, 2, 1);

    glEnableVertexArrayAttrib(m_vao, 3);
    glVertexArrayAttribFormat(m_vao, 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(QuadVertex, tint_r));
    glVertexArrayAttribBinding(m_vao, 3, 0);
    glVertexArrayBindingDivisor(m_vao, 3, 1);

    glEnableVertexArrayAttrib(m_vao, 4);
    glVertexArrayAttribFormat(m_vao, 4, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(QuadVertex, texture_layer));
    glVertexArrayAttribBinding(m_vao, 4, 0);
    glVertexArrayBindingDivisor(m_vao, 4, 1);

    glEnableVertexArrayAttrib(m_vao, 5);
    glVertexArrayAttribFormat(m_vao, 5, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(QuadVertex, texture_unit));
    glVertexArrayAttribBinding(m_vao, 5, 0);
    glVertexArrayBindingDivisor(m_vao, 5, 1);
}

void QuadProgram::destroy() {
    // Delete vertex array and program
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_program);

    // Default states
    m_vao = 0;
    m_program = 0;
    m_matrix_uniform = -1;
}

void QuadProgram::use() const {
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
}

void QuadProgram::bindVertexBuffer(const GLuint buffer) const {
    glVertexArrayVertexBuffer(m_vao, 0, buffer, 0, sizeof(QuadVertex));
}

void QuadProgram::setMatrix(const float* matrix) const {
    glUniformMatrix4fv(m_matrix_uniform, 1, GL_TRUE, matrix);
}

void QuadProgram::draw(const uint32_t start, const uint32_t count) const {
    const auto count_i = static_cast<int32_t>(count);
    glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, count_i, start);
}
