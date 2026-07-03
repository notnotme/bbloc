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
#ifndef COLOR_H
#define COLOR_H


/**
 * @brief Represents an RGBA color.
 *
 * Each component is stored as an 8-bit unsigned byte ranging from 0 to 255.
 */
struct Color {
    uint8_t red;   ///< Red component of the color.
    uint8_t green; ///< Green component of the color.
    uint8_t blue;  ///< Blue component of the color.
    uint8_t alpha; ///< Alpha (transparency) component of the color.
};


#endif //COLOR_H
