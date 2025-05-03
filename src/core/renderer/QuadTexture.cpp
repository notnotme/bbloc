#include "QuadTexture.h"

#include <stdexcept>
#include <vector>


QuadTexture::QuadTexture()
    : m_texture(0) {}

void QuadTexture::create() {
    // Generate the texture
    glGenTextures(1, &m_texture);
    if (m_texture == 0) {
        throw std::runtime_error("Failed to create quad texture");
    }

    // Bind and set default states
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_DEPTH);
}

void QuadTexture::destroy() {
    // Delete texture
    glDeleteTextures(1, &m_texture);

    // Default states
    m_texture = 0;
}

void QuadTexture::bind(const uint8_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
}

void QuadTexture::setUnpackRowLength(const int32_t length) const {
    glPixelStorei(GL_UNPACK_ROW_LENGTH, length);
}

void QuadTexture::setUnpackAlignment(const int32_t alignment) const {
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}

void QuadTexture::blit(const uint8_t x, const uint8_t y, const uint8_t width, const uint8_t height, const uint8_t layer, const void *pixels) const {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, x, y, layer, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

void QuadTexture::clearLayer(const uint8_t layer) {
    const std::vector<uint8_t> pixels(TEXTURE_SIZE * TEXTURE_SIZE, 0);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, TEXTURE_SIZE, TEXTURE_SIZE, 1, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
}

