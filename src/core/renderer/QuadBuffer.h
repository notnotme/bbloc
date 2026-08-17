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
#ifndef QUAD_BUFFER_H
#define QUAD_BUFFER_H

#include <vector>

#include <glad/glad.h>

#include "QuadVertex.h"


/**
 * @brief A buffer for storing and managing quad geometry for rendering.
 *
 * This class provides an interface for inserting textured or tinted quads into a GPU buffer.
 * Quads are staged CPU-side per batch, then uploaded on endBatch(). The GPU buffer grows
 * on demand, so a batch is never truncated.
 *
 * A frame is made of consecutive batches: call resetFrame() once per frame, then for each
 * batch beginBatch() / insert(...) / endBatch(). Each batch must be drawn before the next
 * one begins, as a regrow orphans the previous GPU storage.
 */
class QuadBuffer final {
private:
    /** CPU-side staging storage for the current batch. */
    std::vector<QuadVertex> m_staging;

    /** Handle to the OpenGL vertex buffer object. */
    GLuint m_vertex_buffer;

    /** Current GPU buffer capacity, in quads. */
    uint32_t m_capacity;

    /** Number of quads committed to the GPU buffer since resetFrame(). */
    uint32_t m_frame_count;

    /** Index of the first quad of the current batch within the GPU buffer. */
    uint32_t m_batch_start;

public:
    /** @brief Deleted copy constructor. */
    QuadBuffer(const QuadBuffer &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadBuffer &operator=(const QuadBuffer &) = delete;

    /** @brief Constructs an uninitialized QuadBuffer. */
    explicit QuadBuffer();

    /**
     * @brief Initializes the buffer with an initial quad capacity.
     *
     * The capacity is a starting point: the buffer regrows on demand in endBatch().
     *
     * @param capacity Initial number of quads the buffer supports.
     */
    void create(uint32_t capacity);

    /** @brief Destroys the buffer and releases GPU resources. */
    void destroy();

    /** @brief Starts a new frame: the next batch is placed at the start of the buffer. */
    void resetFrame();

    /**
     * @brief Starts a new batch of quads at the current frame position.
     *
     * @param reserveHint Expected quad count of the batch, used to pre-allocate the staging storage.
     * @return Index of the first quad of this batch, to be used as draw offset.
     */
    uint32_t beginBatch(uint32_t reserveHint = 0);

    /**
     * @brief Uploads the staged quads of the current batch to the GPU buffer.
     *
     * Regrows the GPU buffer (never shrinking) when the batch does not fit. A regrow orphans
     * the previous storage, so batches already uploaded this frame must be drawn beforehand.
     *
     * The staging keeps its content afterwards; only beginBatch() clears it. Sizing the draw of a
     * finished batch must therefore go through the returned count, never through getCount().
     *
     * @return The number of quads this batch uploaded, to be used as draw count.
     */
    uint32_t endBatch();

/**
     * @brief Inserts a plain tinted quad into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param tintR Red component of tint color.
     * @param tintG Green component of tint color.
     * @param tintB Blue component of tint color.
     * @param tintA Alpha component of tint color.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t tintR, uint8_t tintG, uint8_t tintB, uint8_t tintA);

    /**
     * @brief Inserts a textured quad with a full tint (255) into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param textureS Texture UV coordinate S.
     * @param textureT Texture UV coordinate T.
     * @param textureLayer Texture layer index.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t textureS, uint8_t textureT, uint8_t textureLayer);

    /**
     * @brief Inserts a textured and tinted quad into the buffer.
     *
     * @param x X position in pixels.
     * @param y Y position in pixels.
     * @param width Width of the quad.
     * @param height Height of the quad.
     * @param textureS Texture UV coordinate S.
     * @param textureT Texture UV coordinate T.
     * @param textureLayer Texture layer index.
     * @param tintR Red component of tint color.
     * @param tintG Green component of tint color.
     * @param tintB Blue component of tint color.
     * @param tintA Alpha component of tint color.
     * @param textureUnit Which bound atlas texture the quad samples, 0 = the theme atlas.
     */
    void insert(int16_t x, int16_t y, uint16_t width, uint16_t height,
                uint8_t textureS, uint8_t textureT, uint8_t textureLayer,
                uint8_t tintR, uint8_t tintG, uint8_t tintB, uint8_t tintA,
                uint8_t textureUnit = 0);

    /** @brief Returns the OpenGL buffer ID. */
    [[nodiscard]] GLuint getBuffer() const;

    /**
     * @brief Returns the number of quads staged so far in the batch being built.
     *
     * Only meaningful while the batch is still open, to split it into several draws. A batch
     * already closed by endBatch() must be sized with the count endBatch() returned.
     */
    [[nodiscard]] uint32_t getCount() const;
};


#endif //QUAD_BUFFER_H
