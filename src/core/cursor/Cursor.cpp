#include "Cursor.h"


Cursor::Cursor(std::unique_ptr<TextBuffer> buffer)
    : m_buffer(std::move(buffer)),
      m_column(0),
      m_line(0),
      m_is_selection_active(false),
      m_selected_line_start(0),
      m_selected_column_start(0) {}

void Cursor::pageUp(const uint32_t lineCount) {
    // Don't go before 0
    if (m_line > lineCount) {
        m_line -= lineCount;
    } else {
        m_line = 0;
    }

    const auto cursor_string_length = m_buffer->getString(m_line).length();
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
}

void Cursor::pageDown(const uint32_t lineCount) {
    // Don't go after the end
    const auto cursor_line_count = m_buffer->getStringCount() - 1;
    const auto cursor_new_line = m_line + lineCount;
    if (cursor_new_line > cursor_line_count) {
        m_line = cursor_line_count;
    } else {
        m_line = cursor_new_line;
    }

    const auto cursor_string_length = m_buffer->getString(m_line).length();
    if (m_column > cursor_string_length) {
        m_column = cursor_string_length;
    }
}

void Cursor::setName(const std::string_view name) {
    m_name = name;
}

std::string_view Cursor::getName() const {
    return m_name;
}

uint32_t Cursor::getColumn() const {
    return m_column;
}

uint32_t Cursor::getLine() const {
    return m_line;
}

std::optional<TextRange> Cursor::getSelectedRange() const {
    if (!m_is_selection_active) {
        return std::nullopt;
    }

    auto start_line = m_selected_line_start;
    auto start_column = m_selected_column_start;
    auto end_line = m_line;
    auto end_column = m_column;

    if (start_line > end_line) {
        // Invert coordinates totally
        std::swap(start_line, end_line);
        std::swap(start_column, end_column);
    } else if (start_line == end_line && start_column > end_column) {
        // Invert column coordinates
        std::swap(start_column, end_column);
    }

    return TextRange {
        .line_start = start_line,
        .column_start = start_column,
        .line_end = end_line,
        .column_end = end_column
    };
}

std::u16string_view Cursor::getString(const uint32_t line) const {
    return m_buffer->getString(line);
}

uint32_t Cursor::getLineCount() const {
    return m_buffer->getStringCount();
}

std::u16string_view Cursor::getString() const {
    return m_buffer->getString(m_line);
}

void Cursor::moveLeft() {
    if (m_column == 0) {
        // At the very beginning of a line, the cursor can't go left
        if (m_line > 0) {
            // The cursor can go above instead
            m_column = m_buffer->getString(m_line - 1).length();
            --m_line;
        }
    } else {
        --m_column;
    }
}

void Cursor::moveRight() {
    if (m_column == m_buffer->getString(m_line).length()) {
        // At the very end of a line, the cursor can't go right
        if (m_line < m_buffer->getStringCount() - 1) {
            // The cursor can go below instead
            m_column = 0;
            ++m_line;
        }
    } else {
        ++m_column;
    }
}

void Cursor::moveUp() {
    if (m_line > 0) {
        const auto string_above_length = m_buffer->getString(m_line - 1).length();
        if (m_column > string_above_length) {
            // The cursor can't stay at the same X position, put it at the end of the next line
            m_column = string_above_length;
        }
        --m_line;
    } else {
        m_column = 0;
    }
}

void Cursor::moveDown() {
    if (m_line < m_buffer->getStringCount() - 1) {
        const auto string_below_length = m_buffer->getString(m_line + 1).length();
        if (m_column > string_below_length) {
            // The cursor can't stay at the same X position, put it at the end of the next line
            m_column = string_below_length;
        }
        ++m_line;
    } else {
        m_column = m_buffer->getString(m_line).length();
    }
}

void Cursor::moveToStartOfLine() {
    m_column = 0;
}

void Cursor::moveToEndOfLine() {
    m_column = m_buffer->getString(m_line).length();
}

void Cursor::moveToStartOfFile() {
    m_line = 0;
    m_column = 0;
}

void Cursor::moveToEndOfFile() {
    const auto string_count = m_buffer->getStringCount() - 1;
    const auto string_length = m_buffer->getString(string_count).length();
    m_line = string_count;
    m_column = string_length;
}

void Cursor::activateSelection(const bool active) {
    if (active && !m_is_selection_active) {
        m_is_selection_active = true;
        m_selected_line_start = m_line;
        m_selected_column_start = m_column;
    } else if (!active) {
        m_is_selection_active = false;
        m_selected_line_start = 0;
        m_selected_column_start = 0;
    }
}

void Cursor::setPosition(const uint32_t line, const uint32_t column) {
    if (line > m_buffer->getStringCount() - 1) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    if (column > m_buffer->getString(line).length()) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    m_column = column;
    m_line = line;
}

BufferEdit Cursor::insert(const std::u16string_view characters) {
    return m_buffer->insert(m_line, m_column, characters);
}

BufferEdit Cursor::erase(const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) const {
    return m_buffer->erase(lineStart, columnStart, lineEnd, columnEnd);
}

BufferEdit Cursor::newLine() {
    return m_buffer->insert(m_line, m_column, u"\n");
}

std::optional<BufferEdit> Cursor::eraseLeft() {
    if (m_column > 0) {
        // We can erase on the left since column > 0
        const auto& edit = m_buffer->erase(m_line, m_column, m_line, m_column - 1);
        --m_column;
        return edit;
    }

    if (m_line > 0) {
        // We can't erase left because column = 0, so we move the remainder of this line to the end of the line above
        const auto string_above_length = m_buffer->getString(m_line - 1).length();
        const auto& edit =  m_buffer->erase(m_line, m_column, m_line - 1, string_above_length);
        --m_line;
        m_column = string_above_length;
        return edit;
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseRight() {
    if (m_column < m_buffer->getString(m_line).length()) {
        // We can erase on the right since column < string_length
        return  m_buffer->erase(m_line, m_column, m_line, m_column + 1);
    }

    if (m_line < m_buffer->getStringCount() - 1) {
        // We can't erase right because column >= string_length, so we move the line below and append it to this line
        return m_buffer->erase(m_line, m_column, m_line + 1, 0);
    }

    return std::nullopt;
}

std::optional<BufferEdit> Cursor::eraseSelection() const {
    if (!m_is_selection_active) {
        return std::nullopt;
    }

    return m_buffer->erase(m_selected_line_start, m_selected_column_start, m_line, m_column);
}

BufferEdit Cursor::clear() {
    // Reset everything
    m_column = 0;
    m_line = 0;
    m_name = "";

    m_is_selection_active = false;
    m_selected_line_start = 0;
    m_selected_column_start = 0;

    return m_buffer->clear();
}
