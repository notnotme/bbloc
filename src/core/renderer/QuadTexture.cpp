#include "QuadTexture.h"

#include <stdexcept>
#include <vector>


QuadTexture::QuadTexture()
    : m_texture(0) {}

void QuadTexture::create(const uint8_t bindUnit) {
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_texture);
    if (m_texture == 0) {
        throw std::runtime_error("Failed to create quad texture");
    }

    // Bind and set default states
    // Does not sample the border -> CLAMP_TO_EDGE
    // Does not apply any filtering -> NEAREST
    glBindTextureUnit(bindUnit, m_texture);
    glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage3D(m_texture, 1, GL_R8, UINT8_MAX, UINT8_MAX, UINT8_MAX);
}

void QuadTexture::destroy() {
    glDeleteTextures(1, &m_texture);
    m_texture = 0;
}

void QuadTexture::blit(const uint8_t x, const uint8_t y, const uint8_t width, const uint8_t height, const uint8_t layer, const void *pixels) const {
    glTextureSubImage3D(m_texture, 0, x, y, layer, width, height, 1, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

void QuadTexture::clearLayer(const uint8_t layer) const {
    const auto pixels = std::vector<uint8_t>(UINT8_MAX * UINT8_MAX, 0);
    glTextureSubImage3D(m_texture, 0, 0, 0, layer, UINT8_MAX, UINT8_MAX, 1, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
}
