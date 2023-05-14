#include "SpriteShader.h"

#include <glm/gtc/type_ptr.hpp>
#include "../Shader.h"

const char* GLYPH_SHADER_VERTEX_SRC = R"text(
    #version 330 core
    precision lowp float;

    layout (location = 0) in vec2 position;
    layout (location = 1) in vec2 size;
    layout (location = 2) in vec4 texCoord0;
    layout (location = 3) in vec4 color;

    out vec4 inColor;
    out vec2 inTexCoord;

    uniform mat4 projection;

    const ivec2 tlut[4] = ivec2[4] (
        ivec2(2, 1),
        ivec2(0, 1),
        ivec2(2, 3),
        ivec2(0, 3)
    );

    const vec2 plut[4] = vec2[4] (
        vec2( 0.5, -0.5),
        vec2(-0.5, -0.5),
        vec2( 0.5,  0.5),
        vec2(-0.5,  0.5)
    );
    
    void main() {
        mat3 position_mat = mat3 (
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            position.x, position.y, 0.0
        );

        vec3 transformed = position_mat * vec3(plut[gl_VertexID] * size, 1.0);
        gl_Position = projection * vec4(transformed, 1.0);
        inTexCoord = vec2(texCoord0[tlut[gl_VertexID].x], texCoord0[tlut[gl_VertexID].y]);
        inColor = color;
    }
)text";

const char* GLYPH_SHADER_FRAGMENT_SRC = R"text(
    #version 330 core
    precision lowp float;

    in vec4 inColor;
    in vec2 inTexCoord;

    out vec4 fragColor;

    uniform sampler2D tex0;

    void main() {
        /* SDF
        float texelAlpha = texture(tex0, inTexCoord).r;
        float sigDist = max(min(texelAlpha, texelAlpha), min(max(texelAlpha, texelAlpha), texelAlpha));
        float w = fwidth(sigDist);
        float opacity = smoothstep(0.5 - w, 0.5 + w, sigDist);
        fragColor = vec4(inColor.rgb, opacity * inColor.a);
        */
        float texelAlpha = texture(tex0, inTexCoord).r * inColor.a;
        fragColor = vec4(inColor.rgb, texelAlpha);
    }
)text";

SpriteShader::SpriteShader() :
mProgramShaderId(0),
mMatrixUniform(0),
mTextureUniform(0),
mGlyphVertexAttribute({0, 0, 0, 0}) {
}

SpriteShader::~SpriteShader() {
}
 
void SpriteShader::initialize() {
    auto vertexShader = compileShader(GL_VERTEX_SHADER, GLYPH_SHADER_VERTEX_SRC);
    auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, GLYPH_SHADER_FRAGMENT_SRC);

    mProgramShaderId = glCreateProgram();
    glAttachShader(mProgramShaderId, vertexShader);
    glAttachShader(mProgramShaderId, fragmentShader);
    glLinkProgram(mProgramShaderId);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    checkProgram(mProgramShaderId);

    mMatrixUniform = glGetUniformLocation(mProgramShaderId, "projection");
    mTextureUniform = glGetUniformLocation(mProgramShaderId, "tex0");
    mGlyphVertexAttribute.position = glGetAttribLocation(mProgramShaderId, "position");
    mGlyphVertexAttribute.size = glGetAttribLocation(mProgramShaderId, "size");
    mGlyphVertexAttribute.texture = glGetAttribLocation(mProgramShaderId, "texCoord0");
    mGlyphVertexAttribute.color = glGetAttribLocation(mProgramShaderId, "color");
}

void SpriteShader::finalize() {
    glDeleteProgram(mProgramShaderId);
}

void SpriteShader::use() {
    glUseProgram(mProgramShaderId);
}

void SpriteShader::setMatrix(const glm::mat4& matrix) {
    glUniformMatrix4fv(mMatrixUniform, 1, GL_FALSE, glm::value_ptr(matrix));
}

void SpriteShader::setTexture(GLuint texture) {
    glUniform1i(mTextureUniform, texture);
}

SpriteVertexAttribute SpriteShader::vertexAttribute() {
    return mGlyphVertexAttribute;
}
