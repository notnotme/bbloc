#ifndef QUAD_TEXTURE_H
#define QUAD_TEXTURE_H

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

public:
    /** @brief Deleted copy constructor. */
    QuadTexture(const QuadTexture &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadTexture &operator=(const QuadTexture &) = delete;

    /** @brief Constructs an uninitialized QuadTexture object. */
    explicit QuadTexture();

    /**
     * @brief Creates the OpenGL texture array of 256x256x256 pixels and bind it to the OpenGL pipeline.
     *
     * @param bindUnit The unit to bind the texture to.
     */
    void create(uint8_t bindUnit);

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

    /**
     * @brief Clears a texture layer by setting all pixels to zero.
     *
     * @param layer Index of the texture layer to clear.
     */
    void clearLayer(uint8_t layer);
};


#endif //QUAD_TEXTURE_H
