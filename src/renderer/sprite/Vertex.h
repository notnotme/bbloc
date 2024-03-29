#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

/// @brief A GlyphVertex 
struct SpriteVertex {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 texture;
    glm::u8vec4 tint;
};

/// @brief The vertex attributes used by the shader and buffer
struct SpriteVertexAttribute {
    GLint position;
    GLint tint;
    GLint texture;
    GLint size;
};
