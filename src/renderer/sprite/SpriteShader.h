#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Vertex.h"

class SpriteShader {
public:

    SpriteShader();
    virtual ~SpriteShader();

    /// @brief Must be called to initialize the shader internal resources
    void initialize();

    /// @brief Must be called to finalize the shader internal resources
    void finalize();

    /// @brief Make use of this shader, only one shader at a time is possible
    void use();

    /// @brief Set the matrix used to render the buffer
    void setMatrix(const glm::mat4& matrix);

    /// @brief Set the texture unit to use with this shader
    /// @param texture The texture unit
    void setTexture(GLuint texture);

    /// @brief Return the vertex attribute structure of this shader
    SpriteVertexAttribute vertexAttribute();

private:
    
     /// @brief Disallow copy
    SpriteShader(const SpriteShader& copy);
    /// @brief Disallow copy
    SpriteShader& operator=(const SpriteShader&);
    
    /// @brief The OpenGL program shader ID
    GLuint mProgramShaderId;

    /// @brief The matrix uniform of this shader
    GLint mMatrixUniform;

    /// @brief The texture uniform for this shader
    GLint mTextureUniform;

    /// @brief The structur containing the vertex attribute information of this buffer
    SpriteVertexAttribute mGlyphVertexAttribute;

};
