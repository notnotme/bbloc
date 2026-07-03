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
#include "AtlasArray.h"

#include <stdexcept>


AtlasArray::AtlasArray()
    : m_max_row_height(0),
      m_character_layer(0),
      m_next_character_x(0),
      m_next_character_y(0) {}

void AtlasArray::create() {
    // No-op
}

const AtlasEntry &AtlasArray::insert(const char16_t character, const uint32_t width, const uint32_t height, const int32_t bearingX, const int32_t bearingY) {
    if (width > UINT8_MAX || height > UINT8_MAX
        || bearingX < INT8_MIN || bearingX > INT8_MAX
        || bearingY < INT8_MIN || bearingY > INT8_MAX) {
        throw std::runtime_error("AtlasArray::insert Glyph does not fit the texture.");
    }

    const auto glyph_width = static_cast<int32_t>(width);
    const auto glyph_height = static_cast<int32_t>(height);

    // Use a single-neuron algorythm to see if we have room for this character
    if (m_next_character_x + glyph_width > UINT8_MAX) {
        // Does not fit the horizontal axis, increment Y and return to the left
        m_next_character_x = 0;
        m_next_character_y += m_max_row_height;
        m_max_row_height = 0;
    }

    // Check if fits in vertical axis
    if (m_next_character_y + glyph_height > UINT8_MAX) {
        // Does not fit the vertical axis, increment Z and return to the top-left
        m_next_character_x = 0;
        m_next_character_y = 0;
        m_max_row_height = 0;

        ++m_character_layer;
        if (m_character_layer >= UINT8_MAX) {
            // For now, we just throw an exception.
            // If not lazy, implement this:
            // - reset the atlas with bigger values
            // - reset the texture and make it match the new atlas size (check if OpenGL support the new size/depth)
            // - discard the current frame and start a new one.
            throw std::runtime_error("Not enough layers to render character.");
        }
    }

    // Generate the atlas entry
    const auto entry = AtlasEntry {
        .texture_s = static_cast<uint8_t>(m_next_character_x),
        .texture_t = static_cast<uint8_t>(m_next_character_y),
        .layer = m_character_layer,
        .width = static_cast<uint8_t>(width),
        .height = static_cast<uint8_t>(height),
        .bearing_x = static_cast<int8_t>(bearingX),
        .bearing_y = static_cast<int8_t>(bearingY)
    };

    m_next_character_x += glyph_width;
    if (glyph_height > m_max_row_height) {
        m_max_row_height = static_cast<uint8_t>(glyph_height);
    }

    const auto &[new_entry, success] = m_characters.insert({character, entry});
    if (!success) {
        throw std::runtime_error("AtlasArray::insert: failed.");
    }

    return new_entry->second;
}

const AtlasEntry* AtlasArray::get(const char16_t character) const {
    const auto entry = m_characters.find(character);
    if (entry == m_characters.end()) {
        return nullptr;
    }

    return &entry->second;
}

uint8_t AtlasArray::getCurrentLayer() const {
    return m_character_layer;
}

void AtlasArray::clearCharacters() {
    m_characters.clear();
    m_character_layer = 0;
    m_next_character_x = 0;
    m_next_character_y = 0;
    m_max_row_height = 0;
}

void AtlasArray::destroy() {
    // Clear maps
    m_characters.clear();

    // Default states
    m_character_layer = 0;
    m_next_character_x = 0;
    m_next_character_y = 0;
    m_max_row_height = 0;
}
