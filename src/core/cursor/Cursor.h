#ifndef CURSOR_H
#define CURSOR_H

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "buffer/TextBuffer.h"
#include "buffer/BufferEdit.h"
#include "TextRange.h"


/**
 * @brief Represents a text cursor and view into a text buffer.
 *
 * This class manages cursor movement, editing operations,
 * and links to an abstract text buffer for storage.
 */
class Cursor final {
private:
    /** Name of the buffer (filename). */
    std::string m_name;

    /** Pointer to the text buffer backend (owns it). */
    std::unique_ptr<TextBuffer> m_buffer;

    /** Current column (X) position of the cursor. */
    uint32_t m_column;

    /** Current line (Y) position of the cursor. */
    uint32_t m_line;

    /** Holds the state of the selection (active / not active) */
    bool m_is_selection_active;

    /** Holds the index of the line where the selection starts. */
    uint32_t m_selected_line_start;

    /** Holds the index of the column where the selection starts. */
    uint32_t m_selected_column_start;

    /**
     * @brief Erase a range of text inside the internal buffer. This does not move the cursor coordinates.
     *
     * @param lineStart The line index where the range starts.
     * @param columnStart The column index where the range starts.
     * @param lineEnd The line index where the range stops.
     * @param columnEnd The column index where the range stops.
     * @return Reference to the resulting BufferEdit describing the change.
     */
    [[nodiscard]] BufferEdit erase(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const;

public:
    /** @brief Deleted copy constructor. */
    Cursor(const Cursor &) = delete;

    /** @brief Deleted copy assignment operator. */
    Cursor &operator=(const Cursor &) = delete;

    /**
     * @brief Constructs a new Cursor.
     * @param buffer Pointer to the text buffer to manage.
     */
    explicit Cursor(std::unique_ptr<TextBuffer> buffer);

    /** @brief Scrolls the view up by a number of lines. */
    void pageUp(uint32_t lineCount);

    /** @brief Scrolls the view down by a number of lines. */
    void pageDown(uint32_t lineCount);

    /** @brief Sets the name associated with this cursor. */
    void setName(std::string_view name);

    /** @brief Gets the name of the buffer. */
    [[nodiscard]] std::string_view getName() const;

    /** @brief Returns the current column index. */
    [[nodiscard]] uint32_t getColumn() const;

    /** @brief Returns the current line index. */
    [[nodiscard]] uint32_t getLine() const;

    /** @brief Returns The SelectedRange, if available. Always in the right direction (start -> end). */
    [[nodiscard]] std::optional<TextRange> getSelectedRange() const;

    /**
     * @brief Gets the content of a specific line from the buffer.
     *
     * @param line The index of the line to fetch.
     */
    [[nodiscard]] std::u16string_view getString(uint32_t line) const;

    /** @brief Returns the total number of lines in the buffer. */
    [[nodiscard]] uint32_t getLineCount() const;

    /** @brief Returns the current line at the cursor line position (from column 0). */
    [[nodiscard]] std::u16string_view getString() const;

    /** @brief Returns the portion of selected text, if any. */
    [[nodiscard]] std::optional<std::vector<std::u16string_view>> getSelectedText() const;

    /** @brief Moves the cursor one character to the left. Otherwise, move one line above.  */
    void moveLeft();

    /** @brief Moves the cursor one character to the right. Otherwise, move one line below. */
    void moveRight();

    /** @brief Moves the cursor one line up. Otherwise, goes to the start of the line. */
    void moveUp();

    /** @brief Moves the cursor one line down. Otherwise, goes to the end of the line. */
    void moveDown();

    /** @brief Moves the cursor to the start of the current line. */
    void moveToStartOfLine();

    /** @brief Moves the cursor to the end of the current line. */
    void moveToEndOfLine();

    /** @brief Moves the cursor to the start of the file. */
    void moveToStartOfFile();

    /** @brief Moves the cursor to the end of the file. */
    void moveToEndOfFile();

    /**
     * @brief Set the cursor in selection mode. Moves will grow or shrink the selection.
     *
     * @param active if true, the selection is activated, if false, the selection is deactivated and resets its state.
     */
    void activateSelection(bool active);

    /**
     * @brief Sets the new position of the cursor.
     *
     * @param line New line index.
     * @param column New column index.
     * @throw std::runtime_error If coordinates are out of bounds.
     */
    void setPosition(uint32_t line, uint32_t column);

    /**
      * @brief Inserts UTF-16 text at the current cursor position.
      *
      * Moves the cursor forward by the number of inserted characters.
      *
      * @param characters The UTF-16 string to insert.
      * @return The resulting BufferEdit describing the change.
      */
    [[nodiscard]] BufferEdit insert(std::u16string_view characters);

    /**
     * @brief Inserts a line break at the current cursor position.
     *
     * Moves the cursor to the beginning of the next line.
     *
     * @return The resulting BufferEdit describing the change.
     */
    [[nodiscard]] BufferEdit newLine();

    /**
     * @brief Deletes the character immediately before the cursor.
     *
     * Move the cursor backward. It has no effect if the cursor is at the beginning of the buffer.
     *
     * @return An optional BufferEdit describing the change.
     */
    [[nodiscard]] std::optional<BufferEdit> eraseLeft();

    /**
     * @brief Deletes the character immediately after the cursor.
     *
     * It has no effect if the cursor is at the end of the buffer.
     *
     * @return An optional BufferEdit describing the change.
     */
    [[nodiscard]] std::optional<BufferEdit> eraseRight() const;

    /** @return An optional BufferEdit describing the change. */
    [[nodiscard]] std::optional<BufferEdit> eraseSelection();

    /**
     * @brief Clears the entire buffer and resets the cursor to the beginning.
     *
     * After this call, the buffer will be empty and the cursor will be at (0, 0).
     *
     * @return The resulting CursorEdit describing the change.
     */
    [[nodiscard]] BufferEdit clear();
};


#endif //CURSOR_H
