#include "QuadBuffer.h"

#include <stdexcept>


QuadBuffer::QuadBuffer()
    : p_data(nullptr),
      m_vertex_buffer(0),
      m_capacity(0),
      m_count(0) {
}

void QuadBuffer::create(const uint32_t capacity) {
    m_capacity = capacity;

    glGenBuffers(1, &m_vertex_buffer);
    if (m_vertex_buffer == 0) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    const auto size_in_bytes = static_cast<GLsizeiptr>(sizeof(QuadVertex) * m_capacity);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, size_in_bytes, nullptr, GL_DYNAMIC_DRAW);
}

void QuadBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
}

void QuadBuffer::map(const uint32_t start, const uint32_t count) {
    const auto map_offset_in_byte = static_cast<GLintptr>(start * sizeof(QuadVertex));
    const auto count_size_in_byte = static_cast<GLsizeiptr>(count * sizeof(QuadVertex));
    const auto raw_buffer_data = glMapBufferRange(GL_ARRAY_BUFFER, map_offset_in_byte, count_size_in_byte,
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
                                                GL_MAP_FLUSH_EXPLICIT_BIT);

    p_data = static_cast<QuadVertex*>(raw_buffer_data);
    if (p_data == nullptr) {
        throw std::runtime_error("Failed to map vertex buffer");
    }

    m_count = 0;
}

void QuadBuffer::unmap() const {
    const auto count_size_in_bytes = static_cast<GLsizeiptr>(m_count * sizeof(QuadVertex));
    glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, count_size_in_bytes);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t tint_r, const uint8_t tint_g, const uint8_t tint_b, const uint8_t tint_a) {
    auto &vertex = p_data[m_count];
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.tint_r = tint_r;
    vertex.tint_g = tint_g;
    vertex.tint_b = tint_b;
    vertex.tint_a = tint_a;
    vertex.texture_layer = 255;

    ++m_count;
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t texture_s, const uint8_t texture_t, const uint8_t texture_p, const uint8_t texture_q, const uint8_t texture_layer) {
    auto &vertex = p_data[m_count];
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.texture_s = texture_s;
    vertex.texture_t = texture_t;
    vertex.texture_p = texture_p;
    vertex.texture_q = texture_q;
    vertex.tint_r = 255;
    vertex.tint_g = 255;
    vertex.tint_b = 255;
    vertex.tint_a = 255;
    vertex.texture_layer = texture_layer;

    ++m_count;
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t texture_s, const uint8_t texture_t, const uint8_t texture_p, const uint8_t texture_q, const uint8_t texture_layer, const uint8_t tint_r, const uint8_t tint_g, const uint8_t tint_b, const uint8_t tint_a) {
    auto &vertex = p_data[m_count];
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.texture_s = texture_s;
    vertex.texture_t = texture_t;
    vertex.texture_p = texture_p;
    vertex.texture_q = texture_q;
    vertex.tint_r = tint_r;
    vertex.tint_g = tint_g;
    vertex.tint_b = tint_b;
    vertex.tint_a = tint_a;
    vertex.texture_layer = texture_layer;

    ++m_count;
}

void QuadBuffer::destroy() {
    glDeleteBuffers(1, &m_vertex_buffer);
    p_data = nullptr;
    m_vertex_buffer = 0;
    m_capacity = 0;
    m_count = 0;
}

GLuint QuadBuffer::getBuffer() const {
    return m_vertex_buffer;
}

uint32_t QuadBuffer::getCount() const {
    return m_count;
}
