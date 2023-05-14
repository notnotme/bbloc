#pragma once

#include <string>
#include <functional>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

class Font {
public:
    
    /// @brief Default font size
    static const uint8_t defaultHeightPixel = 16;
    
    /// @brief What we got from the character callback parameter
    struct Character {
        /// @brief Size always positive and defined as unsigned short in the freetype library
        glm::u16vec2    size;
        /// @brief Bearing defined as signed int in the freetype library
        glm::ivec2      bearing;
        /// @brief Advance defined as signed long in the freetype library
        int64_t         advance;
        /// @brief The bitmap data is an array of unsigned char
        uint8_t*        bitmap;
    };

    /// @brief Callback definition used by the character function
    typedef std::function<void (const Character&)> CharacterCallback;

    Font();
    virtual ~Font();

    /// @brief Must be called to initialize the Font internal resources
    void initialize();
    
    /// @brief Must be called before forgeting this Font, to cleanup internal resources
    void finalize() const;
    
    /// @brief Load a new font face from a ttf file, throw if cannot load the font
    /// @param path The path of the file to load
    /// @param heightPixel THe height in pixel, saved and used when generating the glyphes
    void load(const std::string path, uint8_t heightPixel = defaultHeightPixel);

    /// @brief Unload the current font, to be called before loading another font face
    void unload() const;

    /// @brief Get a glyph from the current loaded font face, throw if it cannot find the glyph
    /// @param character The character code to get the glyph from
    /// @param callback A callback that allow you to manipulate the requested glyph (data must be copied from it)
    void character(const char16_t character, const CharacterCallback callback) const;

    /// @brief Return the glyph advance and height, throw if it cannot find the glyph
    /// @param character The character code to get the information from
    /// @return A vector with the corresponding data
    glm::u16vec2 glyphMetrics(const char16_t character) const;

    /// @brief Return the current font height in pixels
    /// @return The font height in pixels, or the default font height, if no font is loaded
    uint8_t heightPixel() const;

    /// @brief Set the new font size
    /// @param height The new font height
    void heightPixel(uint8_t height);

private:

    /// @brief Disallow copy
    Font(const Font& copy);
    /// @brief Disallow copy
    Font& operator=(const Font&);

    /// @brief Handle to the Freetype library
    FT_Library  mFreetype;

    /// @brief Handle to the current Freetype Face object loaded
    FT_Face mFace;

    /// @brief The font height in pixel
    uint8_t mHeightPixel;

};
