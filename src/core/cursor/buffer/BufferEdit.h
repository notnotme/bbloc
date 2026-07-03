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
#ifndef BUFFER_EDIT_H
#define BUFFER_EDIT_H

#include <cstdint>


/**
 * @brief Represents a text edit made in the text buffer.
 *
 * This structure stores information about a single edit operation, including the
 * byte offsets and Cursor positions before and after the edit.
 */
struct BufferEdit final {
    /** @brief Represents a line and column position in the text buffer. */
    struct Position final {
        uint32_t line;   ///< Line number (0-based).
        uint32_t column; ///< Column number (0-based).
    };

    uint32_t start_byte;    ///< Byte offset where the edit begins.
    uint32_t old_end_byte;  ///< Byte offset where the replaced region ends before the edit.
    uint32_t new_end_byte;  ///< Byte offset where the new region ends after the edit.

    Position start;         ///< Cursor position where the edit starts.
    Position old_end;       ///< Cursor position where the original content ended.
    Position new_end;       ///< Cursor position where the new content ends.
};


#endif //BUFFER_EDIT_H
