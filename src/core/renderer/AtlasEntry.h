#ifndef ATLAS_ENTRY_H
#define ATLAS_ENTRY_H


/**
 * @brief Represents a single entry in the glyph texture atlas.
 *
 * This structure holds texture coordinates, dimensions, and bearing information
 * used for rendering glyphs.
 */
struct AtlasEntry final {
    uint8_t texture_s;    ///< Horizontal starting UV coordinate (S).
    uint8_t texture_t;    ///< Vertical starting UV coordinate (T).
    uint8_t layer;        ///< Layer index within the atlas texture.
    uint8_t width;        ///< Width of the glyph or sprite in pixels.
    uint8_t height;       ///< Height of the glyph or sprite in pixels.
    int8_t bearing_x;     ///< Horizontal bearing (offset from origin).
    int8_t bearing_y;     ///< Vertical bearing (offset from baseline).
};


#endif //ATLAS_ENTRY_H
