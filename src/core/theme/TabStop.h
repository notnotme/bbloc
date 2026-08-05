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
#ifndef TAB_STOP_H
#define TAB_STOP_H

#include <cstdint>
#include <string_view>


/**
 * @brief Computes the visual column of the tab stop following the given column.
 *
 * Tab stops sit at every multiple of the tab width: a tab drawn at the given column advances
 * to the next multiple, so it is 1 to tabWidth columns wide. Shared by every text walk so the
 * views, the measures and the caret placement can never disagree on where a tab ends.
 *
 * @param visualColumn The visual column the tab is drawn at.
 * @param tabWidth The tab width in columns, at least 1.
 * @return The visual column of the next tab stop.
 */
[[nodiscard]] constexpr uint32_t nextTabStop(const uint32_t visualColumn, const uint32_t tabWidth) {
    return visualColumn - visualColumn % tabWidth + tabWidth;
}

/**
 * @brief Measures the visual width, in columns, of a line prefix starting at column 0.
 *
 * Every character counts one column except tabs, which snap to the next tab stop. Folding the
 * text one tab-delimited run at a time keeps the scan on std::find, so tab-free stretches stay
 * vectorized instead of walking character by character.
 *
 * @param text The line prefix to measure; it must start at visual column 0.
 * @param tabWidth The tab width in columns, at least 1.
 * @return The visual width of the prefix, in columns.
 */
[[nodiscard]] inline uint32_t visualColumns(const std::u16string_view text, const uint32_t tabWidth) {
    const size_t length = text.length();
    uint32_t visual_column = 0;
    size_t position = 0;

    while (position < length) {
        const size_t tab_position = text.find(u'\t', position);
        if (tab_position == std::u16string_view::npos) {
            // No tab left: the remaining characters are one column each
            visual_column += static_cast<uint32_t>(length - position);
            break;
        }

        // Add the tab-free run, then snap the tab to its stop and continue past it
        visual_column = nextTabStop(visual_column + static_cast<uint32_t>(tab_position - position), tabWidth);
        position = tab_position + 1;
    }

    return visual_column;
}


#endif //TAB_STOP_H
