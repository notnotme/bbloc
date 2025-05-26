#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include <string_view>

#include "BufferEdit.h"


/**
 * @brief Abstract base class for representing a text buffer.
 *
 * Provides a common interface for text storage and manipulation, used by the Cursor class.
 * Supports basic operations like line access, insertion, and deletion.
 */
class TextBuffer {
public:
    /** @brief Deleted copy constructor. */
    TextBuffer(const TextBuffer &) = delete;

    /** @brief Deleted copy assignment operator. */
    TextBuffer &operator=(const TextBuffer &) = delete;

    /** @brief Default constructor. */
    explicit TextBuffer() = default;

    /** @brief Virtual destructor. */
    virtual ~TextBuffer() = default;

    /**
     * @brief Returns the content of the given line as a UTF-16 string view.
     *
     * @param line The line number to retrieve.
     * @return The content of the line as std::u16string_view.
     */
    [[nodiscard]] virtual std::u16string_view getString(uint32_t line) const = 0;

    /** @return The total number of lines in the buffer. */
    [[nodiscard]] virtual uint32_t getStringCount() const = 0;

    /**
     * @brief Return the offset of a byte inside the buffer, from a line, column coordinates.
     *
     * @param line The line index of the wanted offset.
     * @param column The column index of the wanted offset.
     * @return The byte offset at this position.
     */
    [[nodiscard]] virtual uint32_t getByteOffset(uint32_t line, uint32_t column) const = 0;

    /**
     * @brief Return the byte count of a range of text inside the buffer.
     *
     * @param lineStart The line index at the beginning of the range.
     * @param columnStart The column index at the beginning of the range.
     * @param lineEnd The line index at the end of the range.
     * @param columnEnd The column index at the end of the range.
     * @return The bytes count for this range.
     */
    [[nodiscard]] virtual uint32_t getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const = 0;

    /**
     * @brief Inserts text at a specified location.
     *
     * @param line Line index where to insert.
     * @param column Column index where to insert.
     * @param characters Characters to insert.
     */
    [[nodiscard]] virtual BufferEdit insert(uint32_t line, uint32_t column, std::u16string_view characters) = 0;

    /**
     * @brief Removes text from the buffer between the given coordinates.
     *
     * @param lineStart Starting line index.
     * @param columnStart Starting column index.
     * @param lineEnd Ending line index.
     * @param columnEnd Ending column index.
     */
    [[nodiscard]] virtual BufferEdit erase(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) = 0;

    /** @brief Clears the entire content of the text buffer. */
    [[nodiscard]] virtual BufferEdit clear() = 0;
};


#endif //TEXT_BUFFER_H
