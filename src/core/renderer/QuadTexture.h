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
#ifndef QUAD_TEXTURE_H
#define QUAD_TEXTURE_H

#include <cstdint>

#include <glad/glad.h>

/**
 * @brief Manages a layered texture used for rendering quads.
 *
 * This class handles an OpenGL texture array that stores glyph pixel data.
 */
class QuadTexture final {
private:
    /** OpenGL handle to the texture array. */
    GLuint m_texture;

    /** Texture unit the texture is bound to for the lifetime of the object. */
    uint8_t m_bind_unit;

public:
    /** @brief Deleted copy constructor. */
    QuadTexture(const QuadTexture &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadTexture &operator=(const QuadTexture &) = delete;

    /** @brief Constructs an uninitialized QuadTexture object. */
    explicit QuadTexture();

    /**
     * @brief Creates the OpenGL texture array of 255x255xlayerCount pixels and bind it to the OpenGL pipeline.
     *
     * @param bindUnit The unit to bind the texture to.
     * @param layerCount Depth of the texture array, in layers.
     */
    void create(uint8_t bindUnit, uint8_t layerCount);

    /** @brief Releases the OpenGL texture resources. */
    void destroy();

    /**
     * @brief Uploads a region of pixels to the texture.
     *
     * @param x X offset within the layer.
     * @param y Y offset within the layer.
     * @param width Width of the region.
     * @param height Height of the region.
     * @param layer Target texture layer.
     * @param pixels Pointer to pixel data (expected to be 8-bit grayscale).
     */
    void blit(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t layer, const void* pixels) const;
};


#endif //QUAD_TEXTURE_H
