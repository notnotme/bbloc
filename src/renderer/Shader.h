#pragma once

#include <string>
#include <glad/glad.h>

/// @brief Helper to compile a shader, throw if the shader cannot be created
/// @param type The type of shader (VERTEX or FRAGMENT)
/// @param src The source code of the shader
/// @return The OpenGL ID associated with this shader
GLuint compileShader(GLenum type, std::string_view src);

/// @brief Check if a program is linked, or throw
/// @param id The OpenGL ID of the program shader to test
void checkProgram(GLuint id);
