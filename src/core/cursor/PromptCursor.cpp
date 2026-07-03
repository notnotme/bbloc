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
        m_column -= charLengthBefore(m_column);
    }
}

void PromptCursor::moveRight() {
    if (m_column < m_string.length()) {
        m_column += charLengthAfter(m_column);
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
        const auto length = charLengthBefore(m_column);
        m_string.erase(m_column - length, length);
        m_column -= length;
    }
}

void PromptCursor::eraseRight() {
    if (m_column < m_string.length()) {
        m_string.erase(m_column, charLengthAfter(m_column));
    }
}

void PromptCursor::clear() {
    m_string.clear();
    m_column = 0;
}

int32_t PromptCursor::charLengthBefore(const int32_t column) const {
    if (column >= 2 && (m_string[column - 1] & 0xFC00) == 0xDC00 && (m_string[column - 2] & 0xFC00) == 0xD800) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}

int32_t PromptCursor::charLengthAfter(const int32_t column) const {
    if (column + 1 < static_cast<int32_t>(m_string.length()) && (m_string[column] & 0xFC00) == 0xD800 && (m_string[column + 1] & 0xFC00) == 0xDC00) {
        // Never split a surrogate pair
        return 2;
    }
    return 1;
}
