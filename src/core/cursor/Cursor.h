#ifndef CURSOR_H
#define CURSOR_H

#include <memory>
#include <string_view>

#include "buffer/TextBuffer.h"
#include "CursorEdit.h"


/**
 * @brief Represents a text cursor and view into a text buffer.
 *
 * This class manages cursor movement, editing operations,
 * and links to an abstract text buffer for storage.
 */
class Cursor final {
private:
    /** @brief Name of the buffer (filename). */
    std::string m_name;

    /** @brief Pointer to the text buffer backend (owns it). */
    std::unique_ptr<TextBuffer> m_buffer;

    /** @brief Current column (X) position of the cursor. */
    int32_t m_column;

    /** @brief Current line (Y) position of the cursor. */
    int32_t m_line;

    /** Holds the last cursor edit */
    CursorEdit m_last_edit;

    [[nodiscard]] uint32_t getStartByte(uint32_t line, uint32_t column) const;

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
    void pageUp(int32_t lineCount);

    /** @brief Scrolls the view down by a number of lines. */
    void pageDown(int32_t lineCount);

    /** @brief Sets the name associated with this cursor. */
    void setName(std::string_view name);

    /** @brief Gets the name of the buffer. */
    [[nodiscard]] std::string_view getName() const;

    /** @brief Returns the current column index. */
    [[nodiscard]] int32_t getColumn() const;

    /** @brief Returns the current line index. */
    [[nodiscard]] int32_t getLine() const;

    /**
     * @brief Gets the content of a specific line from the buffer.
     * @param line The index of the line to fetch.
     */
    [[nodiscard]] std::u16string_view getString(int32_t line) const;

    /** @brief Returns the total number of lines in the buffer. */
    [[nodiscard]] int32_t getLineCount() const;

    /** @brief Returns the current line at the cursor line position (from column 0). */
    [[nodiscard]] std::u16string_view getString() const;

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
     * @brief Sets the new position of the cursor.
     * @param line New line index.
     * @param column New column index.
     */
    void setPosition(int32_t line, int32_t column);

    /**
      * @brief Inserts UTF-16 text at the current cursor position.
      * Moves the cursor forward by the number of inserted characters.
      * @param characters The UTF-16 string to insert.
      * @return Reference to the resulting CursorEdit describing the change.
      */
    const CursorEdit& insert(std::u16string_view characters);

    /**
     * @brief Inserts a line break at the current cursor position.
     * Moves the cursor to the beginning of the next line.
     * @return Reference to the resulting CursorEdit describing the change.
     */
    const CursorEdit& newLine();

    /**
     * @brief Deletes the character immediately before the cursor.
     * Has no effect if the cursor is at the beginning of the buffer.
     * @return Reference to the resulting CursorEdit describing the change.
     */
    const CursorEdit& eraseLeft();

    /**
     * @brief Deletes the character immediately after the cursor.
     * Has no effect if the cursor is at the end of the buffer.
     * @return Reference to the resulting CursorEdit describing the change.
     */
    const CursorEdit& eraseRight();

    /**
     * @brief Clears the entire buffer and resets the cursor to the beginning.
     * After this call, the buffer will be empty and the cursor will be at (0, 0).
     * @return Reference to the resulting CursorEdit describing the change.
     */
    const CursorEdit& clear();
};


#endif //CURSOR_H
