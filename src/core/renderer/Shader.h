#ifndef SHADER_H
#define SHADER_H

#include <string_view>

#include <glad/glad.h>


/**
 * @brief Compiles a GLSL shader of a given type.
 * @param type The type of shader (e.g., GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
 * @param src The GLSL source code.
 * @return The compiled shader ID.
 * @throws std::runtime_error if compilation fails.
 */
GLuint compileShader(GLenum type, std::string_view src);

/**
 * @brief Validates an OpenGL shader program.
 * Checks if the program linked correctly and throws if validation fails.
 * @param id The OpenGL shader program ID.
 * @throws std::runtime_error if validation fails.
 */
void checkProgram(GLuint id);


#endif //SHADER_H
