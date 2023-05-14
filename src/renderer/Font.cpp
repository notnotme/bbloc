#include "Font.h"

#include <stdexcept>

Font::Font() :
mFreetype(nullptr),
mFace(nullptr),
mHeightPixel(Font::defaultHeightPixel) {
}

Font::~Font() {
}

void Font::initialize() {
    FT_Init_FreeType(&mFreetype);
}

void Font::finalize() const {
    FT_Done_FreeType(mFreetype);
}

void Font::load(const std::string path, uint8_t heightPixel) {
    if (FT_New_Face(mFreetype, path.c_str(), 0, &mFace) != 0) {
        throw std::runtime_error(std::string("Font::load FT_New_Face failed: ") + std::string(path));
    }

    FT_Set_Pixel_Sizes(mFace, 0, heightPixel);
    mHeightPixel = heightPixel;
}

void Font::unload() const {
    FT_Done_Face(mFace);
}

uint8_t Font::heightPixel() const {
    return mHeightPixel;
}

void Font::heightPixel(uint8_t height) {
    FT_Set_Pixel_Sizes(mFace, 0, height);
    mHeightPixel = height;
}

void Font::character(const char16_t character, const Font::CharacterCallback callback) const {
    if(FT_Load_Char(mFace, character, FT_LOAD_RENDER) != 0) {
        throw std::runtime_error("Font::getGlyph FT_Load_Char failed");
    }
    
    auto glyph = mFace->glyph;
    callback({
        { glyph->bitmap.width, glyph->bitmap.rows },
        { glyph->bitmap_left, glyph->bitmap_top },
        glyph->advance.x >> 6,
        glyph->bitmap.buffer
    });
}

glm::u16vec2 Font::glyphMetrics(const char16_t character) const {
    if(FT_Load_Char(mFace, character, FT_LOAD_DEFAULT) != 0) {
        throw std::runtime_error("Font::glyphMetrics FT_Load_Char failed");
    }
    // Not the real metrics but this is what we need
    return { mFace->glyph->advance.x >> 6, mHeightPixel };
}
