#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Vertex.h"

class SpriteBuffer {
public:

    /// @brief Buffer usage
    enum Usage {
      STREAM  = GL_STREAM_DRAW,
      DYNAMIC = GL_DYNAMIC_DRAW,
      STATIC  = GL_STATIC_DRAW
    };

    SpriteBuffer(size_t size);
    virtual ~SpriteBuffer();

    /// @brief Must be called to initialize the buffer internal resources
    void initialize(const SpriteVertexAttribute& vertexUniform, Usage usage);

    /// @brief Must be called before forgeting this buffer, to cleanup internal resources
    void finalize();

    /// @brief Make this buffer active (one at a time only is possible)
    void use();

    /// @brief Map the buffer to local memory, make mStorage a valid pointer
    void map();

    /// @brief Unmap this buffer, this flush the modified memory into the gpu
    void unmap();

    /// @brief Return the current number of glyph contained by this buffer
    /// @return current number of glyph
    size_t index();

    /// @brief Return the size of this buffer
    /// @return The size, in glyph count
    size_t size();

    /// @brief Add a glyph into this buffer
    /// @param glyph The glyph to insert
    void add(const SpriteVertex& glyph);

    /// @brief Add a glyph into this buffer
    /// @param glyph The glyph to insert
    void update(size_t index, const SpriteVertex& glyph);

    /// @brief draw the content of the buffer
    void draw();

    /// @brief Draw the content of this buffer with an index
    /// @param start The start index in the buffer
    /// @param count The number of glyph to render
    void draw(size_t start, size_t count);

private:

    /// @brief Allow debug to inspect
    friend class Debug;

      /// @brief Disallow copy
    SpriteBuffer(const SpriteBuffer& copy);
    /// @brief Disallow copy
    SpriteBuffer& operator=(const SpriteBuffer&);

    /// @brief The OpenGL VBO ID of this buffer
    GLuint mVertexBufferId;

    /// @brief The OpenGL VAO ID of this buffer
    GLuint mVertexArrayId;

    /// @brief A pointer to write data into, only valid between map/unmap calls
    SpriteVertex* mStorage;

    /// @brief The capacity of this buffer, in vertices
    size_t mSize;

    /// @brief The number of vertices the buffer contains
    size_t mIndex;

};
