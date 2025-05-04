#include "Cursor.h"


Cursor::Cursor(std::unique_ptr<TextBuffer> buffer)
    : m_buffer(std::move(buffer)),
      m_column(0),
      m_line(0),
      m_last_edit() {}

void Cursor::pageUp(const int32_t lineCount) {
    // Don't go before 0
    m_line = std::max(0, m_line - lineCount);

    const auto cursor_string = m_buffer->getString(m_line);
    const auto cursor_string_length = cursor_string.length();
    if (m_column > cursor_string_length) {
        m_column = static_cast<int32_t>(cursor_string_length);
    }
}

void Cursor::pageDown(const int32_t lineCount) {
    // Don't go after the end
    const auto cursor_line_count = m_buffer->getStringCount();
    m_line = std::min(cursor_line_count - 1, m_line + lineCount);

    const auto cursor_string = m_buffer->getString(m_line);
    const auto cursor_string_length = cursor_string.length();
    if (m_column > cursor_string_length) {
        m_column = static_cast<int32_t>(cursor_string_length);
    }
}

void Cursor::setName(const std::string_view name) {
    m_name = name;
}

std::string_view Cursor::getName() const {
    return m_name;
}

int32_t Cursor::getColumn() const {
    return m_column;
}

int32_t Cursor::getLine() const {
    return m_line;
}

std::u16string_view Cursor::getString(const int32_t line) const {
    return m_buffer->getString(line);
}

int32_t Cursor::getLineCount() const {
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
            const auto string_above = m_buffer->getString(m_line - 1);
            const auto string_above_length = static_cast<int32_t>(string_above.length());
            m_column = string_above_length;
            --m_line;
        }
    } else {
        --m_column;
    }
}

void Cursor::moveRight() {
    const auto string = m_buffer->getString(m_line);
    const auto string_length = string.length();
    if (m_column == string_length) {
        // At the very end of a line, the cursor can't go right
        const auto string_count = m_buffer->getStringCount();
        if (m_line < string_count - 1) {
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
        const auto string_above = m_buffer->getString(m_line - 1);
        const auto string_above_length = static_cast<int32_t>(string_above.length());
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
    const auto string_count = m_buffer->getStringCount();
    if (m_line < string_count - 1) {
        const auto string_below = m_buffer->getString(m_line + 1);
        const auto string_below_length = static_cast<int32_t>(string_below.length());
        if (m_column > string_below_length) {
            // The cursor can't stay at the same X position, put it at the end of the next line
            m_column = string_below_length;
        }
        ++m_line;
    } else {
        const auto string = m_buffer->getString(m_line);
        m_column = static_cast<int32_t>(string.length());
    }
}

void Cursor::moveToStartOfLine() {
    m_column = 0;
}

void Cursor::moveToEndOfLine() {
    const auto string = m_buffer->getString(m_line);
    const auto string_length = string.length();
    m_column = static_cast<int32_t>(string_length);
}

void Cursor::moveToStartOfFile() {
    m_line = 0;
    m_column = 0;
}

void Cursor::moveToEndOfFile() {
    const auto string_count = m_buffer->getStringCount();
    const auto string = m_buffer->getString(string_count - 1);
    const auto string_length = string.length();
    m_line = string_count - 1;
    m_column = static_cast<int32_t>(string_length);
}

void Cursor::setPosition(const int32_t line, const int32_t column) {
    const auto string_count = m_buffer->getStringCount();
    if (line < 0 || line > string_count - 1) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    const auto string = m_buffer->getString(line);
    const auto string_length = string.length();
    if (column < 0 || column > string_length) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    m_column = column;
    m_line = line;
}

const CursorEdit& Cursor::insert(const std::u16string_view characters) {
    m_last_edit.start_byte = getStartByte(m_line, m_column);
    m_last_edit.old_end_byte = m_last_edit.start_byte;
    m_last_edit.new_end_byte = m_last_edit.start_byte + characters.length() * sizeof(char16_t);
    m_last_edit.start.line = m_line;
    m_last_edit.start.column = m_column;
    m_last_edit.old_end.line = m_line;
    m_last_edit.old_end.column = m_column;
    m_buffer->insert(m_line, m_column, characters);

    m_last_edit.new_end.line = m_line;
    m_last_edit.new_end.column = m_column;
    return m_last_edit;
}

const CursorEdit& Cursor::newLine() {
    m_last_edit.start_byte = getStartByte(m_line, m_column);
    m_last_edit.old_end_byte = m_last_edit.start_byte;
    m_last_edit.new_end_byte = m_last_edit.start_byte + sizeof(char16_t);
    m_last_edit.start.line = m_line;
    m_last_edit.start.column = m_column;
    m_last_edit.old_end.line = m_line;
    m_last_edit.old_end.column = m_column;
    m_buffer->insert(m_line, m_column, u"\n");

    m_last_edit.new_end.line = m_line;
    m_last_edit.new_end.column = m_column;
    return m_last_edit;
}

const CursorEdit& Cursor::eraseLeft() {
    m_last_edit.start_byte = getStartByte(m_line, m_column) - sizeof(char16_t);
    m_last_edit.old_end_byte = m_last_edit.start_byte + sizeof(char16_t);
    m_last_edit.new_end_byte = m_last_edit.start_byte;
    m_last_edit.start.line = m_line;
    m_last_edit.start.column = m_column;
    m_last_edit.old_end.line = m_line;
    m_last_edit.old_end.column = m_column;

    if (m_column > 0) {
        // We can erase on the left since column > 0
        m_buffer->erase(m_line, m_column, m_line, m_column - 1);
    } else if (m_line > 0) {
        // We can't erase left because column = 0, so we move the reminder of this line to the end of the line above
        const auto string_above = m_buffer->getString(m_line - 1);
        const auto string_above_length = static_cast<int32_t>(string_above.length());
        m_buffer->erase(m_line, m_column, m_line - 1, string_above_length);
    }

    m_last_edit.new_end.line = m_line;
    m_last_edit.new_end.column = m_column;
    return m_last_edit;
}

const CursorEdit& Cursor::eraseRight() {
    m_last_edit.start_byte = getStartByte(m_line, m_column);
    m_last_edit.old_end_byte = m_last_edit.start_byte + sizeof(char16_t);
    m_last_edit.new_end_byte = m_last_edit.start_byte;
    m_last_edit.start.line = m_line;
    m_last_edit.start.column = m_column;
    m_last_edit.old_end.line = m_line;
    m_last_edit.old_end.column = m_column;

    const auto string = m_buffer->getString(m_line);
    const auto string_length = string.length();
    const auto string_count = m_buffer->getStringCount();
    if (m_column < string_length) {
        // We can erase on the right since column < string_length
        m_buffer->erase(m_line, m_column, m_line, m_column + 1);
    } else if (m_line < string_count - 1) {
        // We can't erase right because column >= string_length, so we move the line below and append it to this line
        m_buffer->erase(m_line, m_column, m_line + 1, 0);
    }

    m_last_edit.new_end.line = m_line;
    m_last_edit.new_end.column = m_column;
    return m_last_edit;
}

const CursorEdit& Cursor::clear() {
    m_buffer->clear();
    m_column = 0;
    m_line = 0;
    m_name = "";

    const auto line_count = m_buffer->getStringCount();
    const auto string = m_buffer->getString(line_count - 1);
    const auto buffer_size = getStartByte(line_count, string.length());

    m_last_edit.start_byte = buffer_size;
    m_last_edit.old_end_byte = buffer_size;
    m_last_edit.new_end_byte = 0;
    m_last_edit.start.line = m_line;
    m_last_edit.start.column = m_column;
    m_last_edit.old_end.line = m_line;
    m_last_edit.old_end.column = m_column;
    m_last_edit.new_end.line = 0;
    m_last_edit.new_end.column = 0;

    return m_last_edit;
}

uint32_t Cursor::getStartByte(const uint32_t line, const uint32_t column) const {
    auto start_byte = column * sizeof(char16_t);
    for (auto i = 0; i < line; ++i) {
        // Don't forget "\n" -> +1 length
        const auto string = m_buffer->getString(i);
        start_byte += (string.length() + 1) * sizeof(char16_t);
    }
    return start_byte;
}
