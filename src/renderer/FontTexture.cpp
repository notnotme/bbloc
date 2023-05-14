#include "FontTexture.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#define TILE_SPACING 1 // Pixel
#define MIN_FONT_SIZE 13 // Pixels
#define MAX_FONT_SIZE 48 // Pixels

FontTexture::FontTexture() :
Texture(),
mFont(std::make_unique<Font>()),
mNextPosition(TILE_SPACING),
mMaxRowHeight(0) {
}

FontTexture::~FontTexture() {
}

void FontTexture::initialize(uint16_t size) {
    Texture::initialize(size);
    mFont->initialize();
}

void FontTexture::setFont(const std::string fontPath) {
    mFont->unload();
    mFont->load(fontPath);
    rebuildGlyphTextureCache();
}

void FontTexture::finalize() {
    clearCharacterCache(false);
    Texture::finalize();

    mFont->unload();
    mFont->finalize();
}

uint8_t FontTexture::fontHeightPixel() const {
    return mFont->heightPixel();
}

bool FontTexture::increaseFontSize() {
    auto currentSize = mFont->heightPixel();
    if (currentSize >= MAX_FONT_SIZE) {
        return false;
    }

    mFont->heightPixel(currentSize + 1);
    rebuildGlyphTextureCache();
    return true;
}

bool FontTexture::decreaseFontSize() {
    auto currentSize = mFont->heightPixel();
    if (currentSize <= MIN_FONT_SIZE) {
        return false;
    }

    mFont->heightPixel(currentSize - 1);
    rebuildGlyphTextureCache();
    return true;
}

bool FontTexture::contains(char16_t character) {
    return mCharacterMap.find(character) != mCharacterMap.end();
}

bool FontTexture::accept(uint16_t width, uint16_t height) {
    // Check current Y - No need for margin, below is void
    if (mNextPosition.y + height > mTextureSize) {
        return false;
    }
    // Check next X/Y
    else if (mNextPosition.x + width + TILE_SPACING > mTextureSize) {
        if (mNextPosition.y + mMaxRowHeight + height + (TILE_SPACING * 2) > mTextureSize) {
            return false;
        }
    }
    return true;
}

void FontTexture::insert(char16_t character, glm::u16vec2 size, glm::ivec2 bearing, int64_t advance, const uint8_t* bitmap) {
    // Assume 8bpp bitmap
    auto pixelCount = size.x * size.y;

    Tile tile = { 
        character, 
        { size.x, size.y },
        bearing,
        advance, 
        { 0, 0, 0, 0 },
        new uint8_t[pixelCount],
        size
    };

    // Always make size even 
    // To avoid pixels jiggling / cropped characters
    tile.size.x = (tile.size.x + 1) & ~1;
    tile.size.y = (tile.size.y + 1) & ~1;
    std::copy(bitmap, bitmap + pixelCount, tile.bitmap);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    copyToTexture(&tile);

    mCharacterMap.emplace(character, tile);
}

void FontTexture::grow() {
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    mTextureSize *= 2;
    if (mTextureSize > maxTextureSize) {
        throw std::runtime_error(std::string("Cannot grow font texture, GL_MAX_TEXTURE_SIZE"));
    }

    auto pixelCount = mTextureSize * mTextureSize;
    uint8_t data[pixelCount];
    std::fill_n(data, pixelCount, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mTextureSize, mTextureSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mTextureSize, mTextureSize, GL_RED, GL_UNSIGNED_BYTE, data);
    createSquare();

    mNextPosition.x = TILE_SPACING;
    mNextPosition.y = TILE_SPACING;
    mMaxRowHeight = 0;

    std::for_each(mCharacterMap.begin(), mCharacterMap.end(), [&](std::pair<const char16_t, Tile>& pair) {
        copyToTexture(&pair.second);
    });
}

void FontTexture::copyToTexture(Tile* tile) {
    if (mNextPosition.x + tile->size.x + TILE_SPACING > mTextureSize) {
        mNextPosition.x = TILE_SPACING;
        mNextPosition.y += mMaxRowHeight + TILE_SPACING;
        mMaxRowHeight = 0;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, mNextPosition.x, mNextPosition.y, tile->bitmapSize.x, tile->bitmapSize.y,
        GL_RED, GL_UNSIGNED_BYTE, tile->bitmap);

    if (tile->size.y > mMaxRowHeight) {
        mMaxRowHeight = tile->size.y;
    }

    tile->texture.s = mNextPosition.x / (float) mTextureSize;
    tile->texture.t = mNextPosition.y / (float) mTextureSize;
    tile->texture.p = tile->texture.s + (tile->size.x / (float) mTextureSize);
    tile->texture.q = tile->texture.t + (tile->size.y / (float) mTextureSize);
    mNextPosition.x += tile->size.x + TILE_SPACING;
}

void FontTexture::get(char16_t character, const FontTexture::TileCallback callback) {
    if (!contains(character)) {
        mFont->character(character, [&](const Font::Character& chr) {
            if (!accept(chr.size.x, chr.size.y)) {
                grow();
            }
            insert(character, chr.size, chr.bearing, chr.advance, chr.bitmap);
        });
    }

    callback(mCharacterMap[character]);
}

void FontTexture::clearCharacterCache(bool reset) {
    auto it = mCharacterMap.begin();
    while (it != mCharacterMap.end()) {
        delete[] it->second.bitmap;
        it = mCharacterMap.erase(it);
    }

    if (!reset) {
        // We likely not use the texture anymore
        return;
    }

    auto pixelCount = mTextureSize * mTextureSize;
    uint8_t data[pixelCount];
    std::fill_n(data, pixelCount, 0);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, mTextureSize, mTextureSize, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mTextureSize, mTextureSize, GL_RED, GL_UNSIGNED_BYTE, data);
    createSquare();

    mNextPosition.x = TILE_SPACING;
    mNextPosition.y = TILE_SPACING;
    mMaxRowHeight = 0;
}

void FontTexture::rebuildGlyphTextureCache() {
    clearCharacterCache(true);

    // TODO: Remove magic numbers, they are common characters used in the ASCII table
    for (auto character = 32; character < 127; ++character) {
        if (!contains(character)) {
            mFont->character(character, [&](const Font::Character& chr) {
                if (! accept(chr.size.x, chr.size.x)) {
                    grow();
                }
                insert(character, chr.size, chr.bearing, chr.advance, chr.bitmap);
            });
        };
    }
}

glm::u32vec2 FontTexture::measure(std::u16string_view line) const {
    glm::u32vec2 size = { 0, 0 };
    for (auto character : line) {
        glm::u16vec2 glyphSize;
        auto tile = mCharacterMap.find(character);
        if (tile != mCharacterMap.end()) {
            glyphSize.x = tile->second.advance;
            glyphSize.y = tile->second.size.y;
        } else {
            // Not found, get the glyph from mFont
            // todo: maybe also cache into FontTexture without the bitmap, then get it later
            glyphSize = mFont->glyphMetrics(character);
        }

        if (character == u'	') {
            // Take in account the TAB (0x0009) character (SPACE width * 4)
            // TODO: extract 4 to param tabSize
            size.x += glyphSize.x * 4;
        } else {
            size.x += glyphSize.x;
        }

        if (size.y < glyphSize.y) {
            size.y = glyphSize.y;
        }
    }
    return size;
}

glm::u32vec2 FontTexture::measure(char16_t character) const {
    glm::u16vec2 glyphSize = { 0, 0 };
    auto tile = mCharacterMap.find(character);
    if (tile != mCharacterMap.end()) {
        glyphSize.x = tile->second.advance;
        glyphSize.y = tile->second.size.y;
    } else {
        // Not found, get the glyph from mFont
        glyphSize = mFont->glyphMetrics(character);
    }

    // Take in account the TAB (0x0009) character (SPACE width * 4)
    if (character == u' ') {
        // TODO: extract 4 to param tabSize
        glyphSize.x = glyphSize.x * 4;
    }
    
    return glyphSize;
}

glm::vec4 FontTexture::squareCoordinates() const {
    return mSquareCoordinates;
}

void FontTexture::createSquare() {
    uint8_t pixel = 0xff;
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RED, GL_UNSIGNED_BYTE, &pixel);

    // Sample in the center of the pixel
    auto halfTextel = (1.0f / mTextureSize) / 2.0f;
    mSquareCoordinates.s = halfTextel;
    mSquareCoordinates.t = halfTextel;
    mSquareCoordinates.p = halfTextel;
    mSquareCoordinates.q = halfTextel;
}
