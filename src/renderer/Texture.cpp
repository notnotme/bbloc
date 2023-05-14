#include "Texture.h"

Texture::Texture() :
mTextureId(0),
mTextureSize(0) {
}

Texture::~Texture() {
}

void Texture::initialize(uint16_t size) {
    mTextureSize = size;
    glGenTextures(1, &mTextureId);
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set all pixels to 0 to not hve garbage inside the texture
    auto pixelCount = mTextureSize * mTextureSize;
    uint8_t data[pixelCount];
    std::fill_n(data, pixelCount, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mTextureSize, mTextureSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mTextureSize, mTextureSize, GL_RED, GL_UNSIGNED_BYTE, data);
}

void Texture::finalize() {
    glDeleteTextures(1, &mTextureId);
}

void Texture::use() const {
    glBindTexture(GL_TEXTURE_2D, mTextureId);
}

uint16_t Texture::size() const {
    return mTextureSize;
}
