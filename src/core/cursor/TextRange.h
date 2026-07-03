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
#ifndef TEXT_RANGE_H
#define TEXT_RANGE_H


/** @brief Represents a range of text by line, column position. */
struct TextRange final {
    uint32_t line_start;     ///< The line where the range starts
    uint32_t column_start;   ///< The column where the range starts
    uint32_t line_end;       ///< The line where the range ends
    uint32_t column_end;     ///< The column where the range ends
};


#endif //TEXT_RANGE_H
