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
    uint8_t texture_p;    ///< Horizontal ending UV coordinate (P).
    uint8_t texture_q;    ///< Vertical ending UV coordinate (Q).
    uint8_t layer;        ///< Layer index within the atlas texture.
    int32_t width;        ///< Width of the glyph or sprite in pixels.
    int32_t height;       ///< Height of the glyph or sprite in pixels.
    int32_t bearing_x;    ///< Horizontal bearing (offset from origin).
    int32_t bearing_y;    ///< Vertical bearing (offset from baseline).
};


#endif //ATLAS_ENTRY_H
