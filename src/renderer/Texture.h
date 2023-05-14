#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Texture {
public:
    Texture();
    virtual ~Texture();

    /// @brief Initialize the texture with an initial size (in pixels), must be called prior to use
    /// @param size The size (squarred) in pixels of the texture
    virtual void initialize(uint16_t size);

    /// @brief Finalize the texture before forgeting it
    virtual void finalize();

    /// @brief Make use of this texture
    void use() const;

    /// @brief get the size of the texture
    /// @return The size (width and height is always the same)
    uint16_t size() const;
  
protected:

    /// @brief Allow debug to inspect
    friend class Debug;

    /// @brief The OpenGL texture ID of this texture
    GLuint mTextureId;

    /// @brief The size of this texture (w & h)
    uint16_t mTextureSize;

private:

    /// @brief Disallow copy
    Texture(const Texture& copy);
    /// @brief Disallow copy
    Texture& operator=(const Texture&);

};
