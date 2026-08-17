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
#ifndef LINE_ENDING_H
#define LINE_ENDING_H

#include <algorithm>
#include <cstdint>
#include <string>


/** @brief The line-ending convention of a buffer: detected when a file is opened, applied when it is saved. */
enum class LineEnding { Lf, Crlf };

/**
 * @brief Picks the convention a file's line-ending counts add up to.
 *
 * CRLF wins only on a strict majority of the newline-terminated lines, so a tie and a file with
 * zero newline-terminated lines (empty, or a single line without a final newline) both read as LF,
 * the convention new buffers start with.
 *
 * @param crlfLineCount The number of newline-terminated lines that ended with CRLF.
 * @param newlineLineCount The number of newline-terminated lines.
 * @return The convention to write the file back with.
 */
[[nodiscard]] inline LineEnding detectLineEnding(const uint32_t crlfLineCount, const uint32_t newlineLineCount) {
    return crlfLineCount * 2 > newlineLineCount ? LineEnding::Crlf : LineEnding::Lf;
}

/**
 * @brief Rewrites an LF-separated text to the given convention.
 *
 * A pass-through for LF; for CRLF, every line feed becomes CRLF. The input never holds a carriage
 * return by construction: reading a file strips them, and the editor never puts one in a line.
 *
 * @param lfText The UTF-8 text with LF line endings, consumed by the rewrite.
 * @param lineEnding The convention to rewrite to.
 * @return The text under the requested convention.
 */
[[nodiscard]] inline std::string applyLineEnding(std::string lfText, const LineEnding lineEnding) {
    if (lineEnding == LineEnding::Lf) {
        return lfText;
    }

    auto crlf_text = std::string{};
    crlf_text.reserve(lfText.length() + static_cast<size_t>(std::count(lfText.begin(), lfText.end(), '\n')));
    for (const auto character : lfText) {
        if (character == '\n') {
            crlf_text.push_back('\r');
        }
        crlf_text.push_back(character);
    }

    return crlf_text;
}


#endif //LINE_ENDING_H
