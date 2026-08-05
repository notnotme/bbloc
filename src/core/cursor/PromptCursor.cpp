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

#include "SurrogatePair.h"


PromptCursor::PromptCursor()
    : m_column(0) {}

uint32_t PromptCursor::getColumn() const {
    return m_column;
}

std::u16string_view PromptCursor::getString() const {
    return m_string;
}

void PromptCursor::moveLeft() {
    if (m_column > 0) {
        m_column -= charLengthBefore(m_string, m_column);
    }
}

void PromptCursor::moveRight() {
    if (m_column < m_string.length()) {
        m_column += charLengthAfter(m_string, m_column);
    }
}

void PromptCursor::moveToStart() {
    m_column = 0;
}

void PromptCursor::moveToEnd() {
    m_column = static_cast<uint32_t>(m_string.length());
}

void PromptCursor::setPosition(const uint32_t column) {
    if (column > m_string.length()) {
        throw std::runtime_error("Cursor::setPosition out of range.");
    }

    m_column = column;
}

void PromptCursor::insert(const std::u16string_view characters) {
    m_string.insert(m_column, characters);
    m_column += static_cast<uint32_t>(characters.length());
}

void PromptCursor::eraseLeft() {
    if (m_column > 0) {
        const auto length = charLengthBefore(m_string, m_column);
        m_string.erase(m_column - length, length);
        m_column -= length;
    }
}

void PromptCursor::eraseRight() {
    if (m_column < m_string.length()) {
        m_string.erase(m_column, charLengthAfter(m_string, m_column));
    }
}

void PromptCursor::clear() {
    m_string.clear();
    m_column = 0;
}
