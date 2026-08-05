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
#ifndef SURROGATE_PAIR_H
#define SURROGATE_PAIR_H

#include <cstdint>
#include <string_view>


/**
 * @brief Measures the character ending at the given column of a line.
 *
 * Shared by every cursor stepping over UTF-16 text, so none of them can split a surrogate pair.
 *
 * @param text The line the column belongs to.
 * @param column The column just after the character.
 * @return 2 when the character is a surrogate pair, 1 otherwise.
 */
[[nodiscard]] inline uint32_t charLengthBefore(const std::u16string_view text, const uint32_t column) {
    if (column >= 2 && (text[column - 1] & 0xFC00) == 0xDC00 && (text[column - 2] & 0xFC00) == 0xD800) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}

/**
 * @brief Measures the character starting at the given column of a line.
 *
 * @param text The line the column belongs to.
 * @param column The column of the character.
 * @return 2 when the character is a surrogate pair, 1 otherwise.
 */
[[nodiscard]] inline uint32_t charLengthAfter(const std::u16string_view text, const uint32_t column) {
    if (column + 1 < text.length() && (text[column] & 0xFC00) == 0xD800 && (text[column + 1] & 0xFC00) == 0xDC00) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}


#endif //SURROGATE_PAIR_H
