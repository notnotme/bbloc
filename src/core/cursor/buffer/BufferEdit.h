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
#include <string_view>


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


/**
 * @brief Returns the position reached by walking a piece of text forward from a starting position.
 *
 * Each line break lands on column 0 of the next line, every other code unit advances the column.
 * Costs the length of the text, not of the buffer, which is what lets the undo history turn a
 * stored chunk back into the range it occupies.
 *
 * @param start The position the text begins at.
 * @param text The text to walk over.
 * @return The position just past the last code unit of the text, start itself when it is empty.
 */
[[nodiscard]] inline BufferEdit::Position advancePosition(const BufferEdit::Position &start, const std::u16string_view text) {
    auto position = start;
    for (const auto unit : text) {
        if (unit == u'\n') {
            ++position.line;
            position.column = 0;
        } else {
            ++position.column;
        }
    }
    return position;
}


#endif //BUFFER_EDIT_H
