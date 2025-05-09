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

const AtlasEntry &AtlasArray::insert(const char16_t character, const uint8_t width, const uint8_t height, const int8_t bearingX, const int8_t bearingY) {
    if (width > UINT8_MAX || height > UINT8_MAX) {
        throw std::runtime_error("AtlasArray::insert Glyph does not fit the texture.");
    }

    // Use a single-neuron algorythm to see if we have room for this character
    if (m_next_character_x + width > UINT8_MAX) {
        // Does not fit the horizontal axis, increment Y and return to the left
        m_next_character_x = 0;
        m_next_character_y += m_max_row_height;
    }

    // Check if fits in vertical axis
    if (m_next_character_y + height > UINT8_MAX) {
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
        .texture_p = static_cast<uint8_t>(m_next_character_x + width),
        .texture_q = static_cast<uint8_t>(m_next_character_y + height),
        .layer = m_character_layer,
        .width = width,
        .height = height,
        .bearing_x = bearingX,
        .bearing_y = bearingY
    };

    m_next_character_x += width;
    if (height > m_max_row_height) {
        m_max_row_height = height;
    }

    const auto &[new_entry, success] = m_characters.insert({character, entry});
    if (!success) {
        throw std::runtime_error("AtlasArray::insert: failed.");
    }

    return new_entry->second;
}

const AtlasEntry* AtlasArray::get(const char16_t character) const {
    if (!m_characters.contains(character)) {
        return nullptr;
    }

    return &m_characters.at(character);
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
