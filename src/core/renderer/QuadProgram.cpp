#include "QuadProgram.h"

#include <cstddef>
#include <stdexcept>

#include "Shader.h"
#include "QuadVertex.h"


static constexpr auto VERTEX_SRC = R"text(
    #version 420 core
    precision lowp float;

    layout (location = 0) in vec2 a_translation;
    layout (location = 1) in vec2 a_size;
    layout (location = 2) in vec2 a_texture;
    layout (location = 3) in vec4 a_tint;
    layout (location = 4) in float a_texture_layer;

    uniform mat4 u_matrix;

    out vec4 v_tint;
    out vec2 v_texture;
    flat out int v_texture_layer;

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
        gl_Position = u_matrix * vec4(position * a_size + a_translation, 0.0, 1.0);
    }
)text";

static constexpr auto FRAGMENT_SRC = R"text(
    #version 420 core
    precision lowp float;

    in vec4 v_tint;
    in vec2 v_texture;
    flat in int v_texture_layer;

    out vec4 o_color;

    layout (binding = 0) uniform sampler2DArray texture_0;

    void main() {
        bool use_texture = v_texture_layer < 255;
        vec4 texel = texture(texture_0, vec3(v_texture, v_texture_layer));
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
    const auto fragment_shader = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SRC);
    const auto vertex_shader = compileShader(GL_VERTEX_SHADER, VERTEX_SRC);
    m_program = glCreateProgram();
    if (m_program == 0) {
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
