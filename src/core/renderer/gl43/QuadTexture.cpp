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
#include "../QuadTexture.h"

#include <stdexcept>


QuadTexture::QuadTexture()
    : m_texture(0),
      m_bind_unit(0) {}

void QuadTexture::create(const uint8_t bindUnit, const uint8_t layerCount) {
    glGenTextures(1, &m_texture);
    if (m_texture == 0) {
        throw std::runtime_error("Failed to create quad texture");
    }

    // Bind and set default states
    // Does not sample the border -> CLAMP_TO_EDGE
    // Does not apply any filtering -> NEAREST
    m_bind_unit = bindUnit;
    glActiveTexture(GL_TEXTURE0 + bindUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, UINT8_MAX, UINT8_MAX, layerCount);
}

void QuadTexture::destroy() {
    glDeleteTextures(1, &m_texture);
    m_texture = 0;
    m_bind_unit = 0;
}

void QuadTexture::blit(const uint8_t x, const uint8_t y, const uint8_t width, const uint8_t height, const uint8_t layer, const void *pixels) const {
    // The texture object stays bound to its own unit; re-activate that unit so the
    // targeted upload reaches this texture and not whichever one was active last.
    glActiveTexture(GL_TEXTURE0 + m_bind_unit);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x, y, layer, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, pixels);
}
