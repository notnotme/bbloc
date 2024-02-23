#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Texture.h"
#include "Font.h"

// TODO: Eventually handle more than one texture,
// then sort characters by textures before drawing
// to minimize texture switches
class FontTexture : public Texture {
public:

    /// @brief What we got from the get function, represent a character inside the texture
    struct Tile {
        /// @brief The character associated with this tile
        char16_t        character;
        /// @brief The size in pixels of the tile (to display)
        glm::u16vec2    size;
        /// @brief The bearing values of this character
        glm::ivec2      bearing;
        /// @brief The advance value of this character
        int64_t         advance;
        /// @brief The texture coordinates for this character inside this texture
        glm::vec4       texture;
        /// @brief The bitmap data of this character
        uint8_t*        bitmap;
        /// @brief The bitmap size (in pixels) of this character
        glm::u16vec2    bitmapSize;
    };

    /// @brief Callback definition used by the tile function
    typedef std::function<void (const Tile&)> TileCallback;

    FontTexture();
    virtual ~FontTexture();

    /// @brief Initialize the texture with an initial size (in pixels), must be called prior to use
    /// @param size The size (squarred) in pixels of the texture
    void initialize(uint16_t size) override;

    /// @brief Set the font to use
    /// @param fontPath The path of the font to use
    void setFont(const std::string fontPath);

    /// @brief Finalize the texture before forgeting it
    void finalize() override;

    /// @brief Get the font height
    /// @return The font height, in pixels
    uint8_t fontHeightPixel() const;

    /// @brief Increase the font size
    /// @return false if the size is at max, true otherwise
    bool increaseFontSize();

    /// @brief Increase the font size
    /// @return false if the size is at min, true otherwise
    bool decreaseFontSize();

    /// @brief Return the size of a line of text
    /// @param line The text to measure
    /// @return The dimension of the text, in pixels
    glm::u32vec2 measure(std::u16string_view line) const;
    
    /// @brief Return the size of a character in pixels
    /// @param character The character to measure
    /// @return The dimension of the character, in pixels
    glm::u32vec2 measure(char16_t character) const;
    
    /// @brief Tell if a character exists inside this texture
    /// @param character The character of the character to request
    /// @return true if contains, false otherwise
    bool contains(char16_t character);

    /// @brief Tell if the dimensions passed in parameter fit inside the texture or not
    /// @param width The width to insert
    /// @param height The height to insert
    /// @return true if the dimension fit, false otherwise
    bool accept(uint16_t width, uint16_t height);

    /// @brief Insert a new character inside the texture
    /// @param character The character to associate with this character
    /// @param size The size of the new character
    /// @param bearing The bearing of the character
    /// @param advance The advance of the character
    /// @param bitmap The bitmap data (uint8_t*)
    void insert(char16_t character, glm::u16vec2 size, glm::ivec2 bearing, int64_t advance, const uint8_t* bitmap);

    /// @brief Return a Tile corresponding to the requested character
    /// @param character The character of the character to request
    /// @return A Tile* with the corresponding data, or nullptr
    void get(char16_t character, const TileCallback callback);

    /// @brief Return the texture coordinates of the 0xff pixel inside the texture
    /// @return The texture coordinates of the pixel
    glm::vec4 pixelCoordinates() const;

private:

    /// @brief Allow debug to inspect
    friend class Debug;

    /// @brief Disallow copy
    FontTexture(const FontTexture& copy);
    /// @brief Disallow copy
    FontTexture& operator=(const FontTexture&);
  
    /// @brief The font object used by the renderer
    std::unique_ptr<Font> mFont;

    /// @brief The unordered_map containing all TIle
    std::unordered_map<char16_t, Tile> mCharacterMap;

    /// @brief The next possible position that could be used to insert the next character
    glm::u16vec2 mNextPosition;
   
    /// @brief  The maximum height of a row of character
    uint16_t mMaxRowHeight;

    /// @brief The coordinates of the filled square inside the texture
    glm::vec4 mPixelCoordinates;

    /// @brief Grow the texture by a power of two, the already existing characters are kept
    void grow();

    /// @brief Clear all character already cached inside this texture, can reset the texture to black with no chatracters
    /// @param reset if true, reset texture to full 0x00 and create the pixel / square coordinate
    void clearCharacterCache(bool reset);

    /// @brief To rebuild the texture cache 
    void rebuildGlyphTextureCache();

    /// @brief Copy a new tile inside the texture and calculate his texture coordinates
    /// @param tile The tile to copy inside the texture data
    void copyToTexture(Tile* tile);

    /// @brief Create a white pixel inside the texture and set mPixelCoordinates values
    void createPixel();

};
