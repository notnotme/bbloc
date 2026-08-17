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
#ifndef LINE_SCANNER_H
#define LINE_SCANNER_H

#include <cstddef>
#include <string>
#include <string_view>


/**
 * @brief Scans buffer lines for one term, folding case once per term and once per line.
 *
 * The case-insensitive comparison is the ASCII-only fold of toLowerAscii, applied to both sides
 * exactly as the per-position comparison it replaces did; matching then runs on the folded copies,
 * so a single find/rfind replaces the hand-rolled quadratic scan.
 */
class LineScanner final {
private:
    /** The term to look for, ASCII-folded when the comparison is case-insensitive. */
    std::u16string m_term;

    /** Scratch holding the folded copy of the current line; unused when the comparison is case-sensitive. */
    std::u16string m_folded_line;

    /** The line being scanned: the caller's view, or a view over m_folded_line. */
    std::u16string_view m_line;

    /** true when the comparison is case-sensitive and no folding happens. */
    const bool m_case_sensitive;

    /**
     * @brief Lower-cases an ASCII letter, leaving other code units untouched.
     *
     * Only A-Z are folded; this is the deliberate ASCII-only limitation of the v1 case-insensitive matching.
     *
     * @param character The code unit to fold.
     * @return The lower-cased code unit.
     */
    [[nodiscard]] static char16_t toLowerAscii(char16_t character);

public:
    /**
     * @brief Builds a scanner for one term under one case-sensitivity mode.
     * @param term The term to look for.
     * @param caseSensitive Whether the comparison is case-sensitive.
     */
    explicit LineScanner(std::u16string_view term, bool caseSensitive);

    /**
     * @brief Sets the line the next lookups run on, folding it once when needed.
     *
     * The scanner keeps a view into the line, not a copy, so the line must outlive the lookups run on it.
     *
     * @param line The line content to scan.
     */
    void setLine(std::u16string_view line);

    /**
     * @brief Finds the first occurrence of the term at or after an offset on the current line.
     * @param from The column offset to start scanning from.
     * @return The starting column, or std::u16string_view::npos when absent.
     */
    [[nodiscard]] size_t indexOf(size_t from) const;

    /**
     * @brief Finds the last occurrence of the term starting before a bound on the current line.
     * @param limit The exclusive upper bound for the match start column.
     * @return The starting column, or std::u16string_view::npos when absent.
     */
    [[nodiscard]] size_t lastIndexOf(size_t limit) const;

    /** @return The term length in code units. */
    [[nodiscard]] size_t termLength() const;

    /**
     * @brief Tells whether the term can overlap itself, i.e. a proper prefix of it is also a suffix.
     *
     * scanMatches enumerates non-overlapping occurrences, so a term that cannot overlap itself has
     * every one of its occurrences in that enumeration. Backward stepping relies on it.
     *
     * @return true when two occurrences of the term can overlap.
     */
    [[nodiscard]] bool isSelfOverlapping() const;
};


#endif //LINE_SCANNER_H
