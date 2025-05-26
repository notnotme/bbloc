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
