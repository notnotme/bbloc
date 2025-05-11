#ifndef QUAD_PROGRAM_H
#define QUAD_PROGRAM_H

#include <glad/glad.h>


/**
 * @brief Manages a simple shader program for rendering textured quads.
 *
 * This class encapsulates an OpenGL program and its associated vertex array object,
 * providing methods to bind and configure the rendering pipeline.
 */
class QuadProgram final {
private:
    /** Handle to the vertex array object. */
    GLuint m_vao;

    /** Handle to the compiled OpenGL shader program. */
    GLuint m_program;

    /** Handle to the matrix uniform location used for transformations. */
    GLint m_matrix_uniform;

public:
    /** @brief Deleted copy constructor. */
    QuadProgram(const QuadProgram &) = delete;

    /** @brief Deleted copy assignment operator. */
    QuadProgram &operator=(const QuadProgram &) = delete;

    /** @brief Constructs an uninitialized QuadProgram object. */
    explicit QuadProgram();

    /** @brief Creates and compiles the shader program and associated VAO. */
    void create();

    /** @brief Releases the OpenGL program and VAO resources. */
    void destroy();

    /** @brief Sets this program as the current one in the OpenGL pipeline. */
    void use() const;

    /**
     * @brief Binds a vertex buffer to the shader's attribute layout.
     * @param buffer OpenGL buffer object to bind.
     */
    void bindVertexBuffer(GLuint buffer) const;

    /**
     * @brief Uploads a 4x4 transformation matrix to the shader program.
     * @param matrix Pointer to 16 floats representing the matrix.
     */
    void setMatrix(const float* matrix) const;

    /**
     * @brief Issues a draw call to render a range of quads from the vertex buffer.
     * @param start First index of the vertex buffer.
     * @param count Number of quads to render.
     */
    void draw(uint32_t start, uint32_t count) const;
};


#endif //QUAD_PROGRAM_H
