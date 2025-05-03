#include "PromptCursor.h"

#include <stdexcept>


PromptCursor::PromptCursor()
    : m_column(0) {}

int32_t PromptCursor::getColumn() const {
    return m_column;
}

std::u16string_view PromptCursor::getString() const {
    return m_string;
}

void PromptCursor::moveLeft() {
    if (m_column > 0) {
        --m_column;
    }
}

void PromptCursor::moveRight() {
    if (m_column < m_string.length()) {
        ++m_column;
    }
}

void PromptCursor::moveToStart() {
    m_column = 0;
}

void PromptCursor::moveToEnd() {
    m_column = static_cast<int32_t>(m_string.length());
}

void PromptCursor::setPosition(const int32_t column) {
    if (column < 0 || column > m_string.length()) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    m_column = column;
}

void PromptCursor::insert(const std::u16string_view characters) {
    m_string.insert(m_column, characters);
    m_column += static_cast<int32_t>(characters.length());
}

void PromptCursor::eraseLeft() {
    if (m_column > 0) {
        m_string.erase(m_column - 1, 1);
        --m_column;
    }
}

void PromptCursor::eraseRight() {
    if (m_column < m_string.length()) {
        m_string.erase(m_column, 1);
    }
}

void PromptCursor::clear() {
    m_string.clear();
    m_column = 0;
}
