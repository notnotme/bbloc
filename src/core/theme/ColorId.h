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
#ifndef COLOR_ID_H
#define COLOR_ID_H

#include <cstddef>


/**
 * @brief Enumeration of color identifiers used in the UI.
 *
 * These identifiers are used to access theme colors for UI components.
 */
enum class ColorId {
    MarginBackground,       ///< Background color of the margin area (line number container).
    LineBackground,         ///< Background color for the current (at cursor position) text lines.
    SelectedTextBackground, ///< Background color for selected text range.
    LineNumber,             ///< Color for the line numbers.
    InfoBarBackground,      ///< Background color of the info bar.
    EditorBackground,       ///< Background color of the editor area.
    PromptBackground,       ///< Background color of the command prompt.
    InfoBarText,            ///< Text color in the info bar.
    PromptText,             ///< Text color for static prompt messages.
    PromptInputText,        ///< Text color for user input in the prompt.
    Border,                 ///< Color used for borders (e.g. between components).
    CursorIndicator         ///< Color of the cursor indicator.
};

/** @brief Number of ColorId values; must track the last enumerator of ColorId. */
inline constexpr std::size_t COLOR_ID_COUNT = static_cast<std::size_t>(ColorId::CursorIndicator) + 1;


#endif //COLOR_ID_H
