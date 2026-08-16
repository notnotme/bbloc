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
#include "LineScanner.h"

LineScanner::LineScanner(const std::u16string_view term, const bool caseSensitive)
    : m_term(term),
      m_case_sensitive(caseSensitive) {
    if (!m_case_sensitive) {
        // Fold the term once, instead of folding both sides at every candidate position.
        for (auto &character : m_term) {
            character = toLowerAscii(character);
        }
    }
}

void LineScanner::setLine(const std::u16string_view line) {
    if (m_case_sensitive) {
        m_line = line;
        return;
    }

    // Fold the line once into the scratch; every lookup on it then runs on the folded copy.
    m_folded_line.assign(line);
    for (auto &character : m_folded_line) {
        character = toLowerAscii(character);
    }

    m_line = m_folded_line;
}

size_t LineScanner::indexOf(const size_t from) const {
    // Both sides are folded, so find carries the same semantics as the per-position comparison:
    // in particular an empty term matches at from whenever it fits within the line.
    return m_line.find(m_term, from);
}

size_t LineScanner::lastIndexOf(const size_t limit) const {
    if (limit == 0) {
        return std::u16string_view::npos;
    }

    // rfind returns the greatest start position <= limit - 1, i.e. strictly before the limit.
    return m_line.rfind(m_term, limit - 1);
}

size_t LineScanner::termLength() const {
    return m_term.length();
}

bool LineScanner::isSelfOverlapping() const {
    // A shift that leaves the term matching itself is exactly an overlap of two occurrences.
    const auto term = std::u16string_view { m_term };
    for (size_t shift = 1; shift < term.length(); ++shift) {
        if (term.substr(shift) == term.substr(0, term.length() - shift)) {
            return true;
        }
    }

    return false;
}

char16_t LineScanner::toLowerAscii(const char16_t character) {
    if (character >= u'A' && character <= u'Z') {
        return static_cast<char16_t>(character - u'A' + u'a');
    }

    return character;
}
