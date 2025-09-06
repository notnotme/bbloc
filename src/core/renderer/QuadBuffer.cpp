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
#include "QuadBuffer.h"

#include <algorithm>
#include <stdexcept>


QuadBuffer::QuadBuffer()
    : m_vertex_buffer(0),
      m_capacity(0),
      m_frame_count(0),
      m_batch_start(0) {
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
    m_staging.reserve(m_capacity);
}

void QuadBuffer::resetFrame() {
    m_frame_count = 0;
}

uint32_t QuadBuffer::beginBatch(const uint32_t reserveHint) {
    m_batch_start = m_frame_count;
    m_staging.clear();
    if (reserveHint > 0) {
        m_staging.reserve(reserveHint);
    }

    return m_batch_start;
}

uint32_t QuadBuffer::endBatch() {
    const auto batch_count = static_cast<uint32_t>(m_staging.size());
    const auto needed_capacity = m_batch_start + batch_count;
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    if (needed_capacity > m_capacity) {
        // Regrow by orphaning: previous batches of this frame are already drawn, their storage
        // is kept alive by the driver until those draws complete.
        m_capacity = std::max(needed_capacity, m_capacity * 2);
        const auto capacity_in_bytes = static_cast<GLsizeiptr>(sizeof(QuadVertex) * m_capacity);
        glBufferData(GL_ARRAY_BUFFER, capacity_in_bytes, nullptr, GL_DYNAMIC_DRAW);
    }

    if (batch_count > 0) {
        const auto batch_offset_in_bytes = static_cast<GLintptr>(m_batch_start * sizeof(QuadVertex));
        const auto batch_size_in_bytes = static_cast<GLsizeiptr>(batch_count * sizeof(QuadVertex));
        glBufferSubData(GL_ARRAY_BUFFER, batch_offset_in_bytes, batch_size_in_bytes, m_staging.data());
    }

    m_frame_count = needed_capacity;
    return batch_count;
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t tint_r, const uint8_t tint_g, const uint8_t tint_b, const uint8_t tint_a) {
    auto &vertex = m_staging.emplace_back();
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.tint_r = tint_r;
    vertex.tint_g = tint_g;
    vertex.tint_b = tint_b;
    vertex.tint_a = tint_a;
    vertex.texture_layer = 255;
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t texture_s, const uint8_t texture_t, const uint8_t texture_layer) {
    auto &vertex = m_staging.emplace_back();
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.texture_s = texture_s;
    vertex.texture_t = texture_t;
    vertex.tint_r = 255;
    vertex.tint_g = 255;
    vertex.tint_b = 255;
    vertex.tint_a = 255;
    vertex.texture_layer = texture_layer;
}

void QuadBuffer::insert(const int16_t x, const int16_t y, const uint16_t width, const uint16_t height, const uint8_t texture_s, const uint8_t texture_t, const uint8_t texture_layer, const uint8_t tint_r, const uint8_t tint_g, const uint8_t tint_b, const uint8_t tint_a) {
    auto &vertex = m_staging.emplace_back();
    vertex.translation_x = x;
    vertex.translation_y = y;
    vertex.width = width;
    vertex.height = height;
    vertex.texture_s = texture_s;
    vertex.texture_t = texture_t;
    vertex.tint_r = tint_r;
    vertex.tint_g = tint_g;
    vertex.tint_b = tint_b;
    vertex.tint_a = tint_a;
    vertex.texture_layer = texture_layer;
}

void QuadBuffer::destroy() {
    glDeleteBuffers(1, &m_vertex_buffer);
    m_staging.clear();
    m_staging.shrink_to_fit();
    m_vertex_buffer = 0;
    m_capacity = 0;
    m_frame_count = 0;
    m_batch_start = 0;
}

GLuint QuadBuffer::getBuffer() const {
    return m_vertex_buffer;
}

uint32_t QuadBuffer::getCount() const {
    return static_cast<uint32_t>(m_staging.size());
}
