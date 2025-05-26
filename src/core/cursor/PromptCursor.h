#ifndef PROMPT_CURSOR_H
#define PROMPT_CURSOR_H

#include <string>
#include <string_view>


/**
 * @brief Represents a single-line cursor for command input.
 *
 * Used in the prompt bar for entering command strings with cursor movement,
 * text editing, and insertion support.
 */
class PromptCursor final {
private:
    /** Internal text buffer storing the command input. */
    std::u16string m_string;

    /** Current column position of the cursor in the buffer. */
    int32_t m_column;

public:
    /** @brief Deleted copy constructor. */
    PromptCursor(const PromptCursor &) = delete;

    /** @brief Deleted copy assignment operator. */
    PromptCursor &operator=(const PromptCursor &) = delete;

    /** @brief Default constructor initializing an empty cursor. */
    explicit PromptCursor();

    /** @brief Returns the current column of the cursor position. */
    [[nodiscard]] int32_t getColumn() const;

    /** @brief Returns the current string content of the prompt. */
    [[nodiscard]] std::u16string_view getString() const;

    /** @brief Moves the cursor one position to the left. */
    void moveLeft();

    /** @brief Moves the cursor one position to the right. */
    void moveRight();

    /** @brief Moves the cursor to the beginning of the string. */
    void moveToStart();

    /** @brief Moves the cursor to the end of the string. */
    void moveToEnd();

    /**
     * @brief Sets the cursor column position.
     *
     * @param column Desired column position (will be clamped to bounds).
     */
    void setPosition(int32_t column);

    /**
     * @brief Inserts characters at the current cursor position.
     *
     * Moves the cursor forward by the number of inserted characters.
     *
     * @param characters UTF-16 string to insert.
     */
    void insert(std::u16string_view characters);

    /**
     * @brief Erases the character before the cursor (backspace).
     *
     * Move the cursor backward. It has no effect if the cursor is at the beginning of the buffer.
     */
    void eraseLeft();

    /**
     * @brief Erases the character at the current cursor position (delete).
     *
     * It has no effect if the cursor is at the end of the buffer.
     */
    void eraseRight();

    /**
     * @brief Clears the cursor text and resets the position.
     *
     * After this call, the buffer will be empty and the cursor will be at position 0.
     */
    void clear();
};


#endif //PROMPT_CURSOR_H
