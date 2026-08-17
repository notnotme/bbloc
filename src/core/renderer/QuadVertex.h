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
#ifndef QUAD_VERTEX_H
#define QUAD_VERTEX_H

#include <cstdint>


/**
 * @brief Represents a single vertex used for rendering a textured quad.
 *
 * This structure is used to describe the geometry and visual appearance of a rectangle
 * to be drawn on screen using a texture atlas.
 */
struct QuadVertex final {
    int16_t translation_x = 0;   /**< X translation (in pixels) from the origin. */
    int16_t translation_y = 0;   /**< Y translation (in pixels) from the origin. */
    uint16_t width = 0;          /**< Width of the quad in pixels. */
    uint16_t height = 0;         /**< Height of the quad in pixels. */
    uint8_t texture_s = 0;       /**< Texture coordinate S (left) */
    uint8_t texture_t = 0;       /**< Texture coordinate T (top). */
    uint8_t tint_r = 0;          /**< Tint color red component. */
    uint8_t tint_g = 0;          /**< Tint color green component. */
    uint8_t tint_b = 0;          /**< Tint color blue component. */
    uint8_t tint_a = 0;          /**< Tint color alpha component. */
    uint8_t texture_layer = 0;   /**< Index of the texture layer in the atlas. */
    uint8_t texture_unit = 0;    /**< Which bound atlas texture the quad samples, 0 = the theme atlas. */
};


#endif //QUAD_VERTEX_H
