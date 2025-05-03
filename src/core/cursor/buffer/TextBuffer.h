#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include <cstdint>
#include <string_view>


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
     * @param line The line number to retrieve.
     * @return The content of the line as std::u16string_view.
     */
    [[nodiscard]] virtual std::u16string_view getString(int32_t line) const = 0;

    /** @return The total number of lines in the buffer. */
    [[nodiscard]] virtual int32_t getStringCount() const = 0;

    /**
     * @brief Inserts text at a specified location.
     * @param line Line index where to insert.
     * @param column Column index where to insert.
     * @param characters Characters to insert.
     */
    virtual void insert(int32_t& line, int32_t& column, std::u16string_view characters) = 0;

    /**
     * @brief Removes text from the buffer between the given coordinates.
     * @param lineStart Starting line index.
     * @param columnStart Starting column index.
     * @param lineEnd Ending line index.
     * @param columnEnd Ending column index.
     */
    virtual void erase(int32_t& lineStart, int32_t& columnStart, int32_t lineEnd, int32_t columnEnd) = 0;

    /** @brief Clears the entire content of the text buffer. */
    virtual void clear() = 0;
};


#endif //TEXT_BUFFER_H
