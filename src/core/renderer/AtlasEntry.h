/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
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
