#ifndef QUAD_TEXTURE_H
#define QUAD_TEXTURE_H

#include <glad/glad.h>

/**
 * @brief Manages a layered texture used for rendering quads.
 *
 * This class handles an OpenGL texture array that stores glyph pixel data.
 */
class QuadTexture final {
public:
    /** Maximum size of the texture in width and height (in pixels). */
    static constexpr auto TEXTURE_SIZE = 255;

    /** Maximum number of texture layers. */
    static constexpr auto TEXTURE_DEPTH = 16;

private:
    /** OpenGL handle to the texture array. */
    GLuint m_texture;

public:
    /** @brief Deleted copy constructor. */
    QuadTexture(const QuadTexture &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadTexture &operator=(const QuadTexture &) = delete;

    /** @brief Constructs an uninitialized QuadTexture object. */
    explicit QuadTexture();

    /** @brief Creates the OpenGL texture array. */
    void create();

    /** @brief Releases the OpenGL texture resources. */
    void destroy();

    /**
     * @brief Binds this texture to a given texture unit in the OpenGL pipeline.
     * @param unit Texture unit index to bind to.
     */
    void bind(uint8_t unit) const;

    /**
     * @brief Sets the unpack row length (stride) used when uploading texture data.
     * @param length Number of pixels per row.
     */
    void setUnpackRowLength(int32_t length) const;

    /**
     * @brief Sets the unpack alignment used by glTexSubImage3D.
     * @param alignment Byte alignment (commonly 1, 4, or 8).
     */
    void setUnpackAlignment(int32_t alignment) const;

    /**
     * @brief Uploads a region of pixels to the texture.
     * @param x X offset within the layer.
     * @param y Y offset within the layer.
     * @param width Width of the region.
     * @param height Height of the region.
     * @param layer Target texture layer.
     * @param pixels Pointer to pixel data (expected to be 8-bit grayscale).
     */
    void blit(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t layer, const void* pixels) const;

    /**
     * @brief Clears a texture layer by setting all pixels to zero.
     * @param layer Index of the texture layer to clear.
     */
    void clearLayer(uint8_t layer);
};


#endif //QUAD_TEXTURE_H
