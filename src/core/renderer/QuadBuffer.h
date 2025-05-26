#ifndef QUAD_BUFFER_H
#define QUAD_BUFFER_H

#include <glad/glad.h>

#include "QuadVertex.h"


/**
 * @brief A buffer for storing and managing quad geometry for rendering.
 *
 * This class provides an interface for inserting textured or tinted quads into a GPU buffer.
 */
class QuadBuffer final {
private:
    /** Pointer to the CPU-side mapped vertex data. */
    QuadVertex *p_data;

    /** Handle to the OpenGL vertex buffer object. */
    GLuint m_vertex_buffer;

    /** Maximum number of quads the buffer can hold. */
    uint32_t m_capacity;

    /** Number of quads currently inserted into the mapped range of the buffer. */
    uint32_t m_count;

public:
    /** @brief Deleted copy constructor. */
    QuadBuffer(const QuadBuffer &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadBuffer &operator=(const QuadBuffer &) = delete;

    /** @brief Constructs an uninitialized QuadBuffer. */
    explicit QuadBuffer();

    /**
     * @brief Initializes the buffer with a specific quad capacity.
     *
     * @param capacity Number of quads the buffer should support.
     */
    void create(uint32_t capacity);

    /** @brief Destroys the buffer and releases GPU resources. */
    void destroy();

    /**
     * @brief Maps a portion of the buffer to CPU address space.
     *
     * @param start Index of the first quad to map.
     * @param count Number of quads to map.
     */
    void map(uint32_t start, uint32_t count);

    /** @brief Unmaps and flushes the buffer, uploading data to the GPU. */
    void unmap() const;

/**
     * @brief Inserts a plain tinted quad into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param tint_r Red component of tint color.
     * @param tint_g Green component of tint color.
     * @param tint_b Blue component of tint color.
     * @param tint_a Alpha component of tint color.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t tint_r, uint8_t tint_g, uint8_t tint_b, uint8_t tint_a);

    /**
     * @brief Inserts a textured quad with a full tint (255) into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param texture_s Texture UV coordinate S.
     * @param texture_t Texture UV coordinate T.
     * @param texture_layer Texture layer index.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t texture_s, uint8_t texture_t, uint8_t texture_layer);

    /**
     * @brief Inserts a textured and tinted quad into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param texture_s Texture UV coordinate S.
     * @param texture_t Texture UV coordinate T.
     * @param texture_layer Texture layer index.
     * @param tint_r Red component of tint color.
     * @param tint_g Green component of tint color.
     * @param tint_b Blue component of tint color.
     * @param tint_a Alpha component of tint color.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t texture_s, uint8_t texture_t, uint8_t texture_layer,
                uint8_t tint_r, uint8_t tint_g, uint8_t tint_b, uint8_t tint_a);

    /** @brief Returns the OpenGL buffer ID. */
    [[nodiscard]] GLuint getBuffer() const;

    /** @brief Returns the number of quads currently inserted into the buffer. */
    [[nodiscard]] uint32_t getCount() const;
};


#endif //QUAD_BUFFER_H
