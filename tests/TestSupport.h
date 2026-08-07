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
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include <utf8.h>

#include "doctest.h"

#include "core/cursor/Cursor.h"
#include "core/cursor/buffer/LineBuffer.h"

namespace doctest {

/**
 * @brief Prints a UTF-16 buffer as UTF-8 so a failing CHECK shows the text rather than "{?}".
 *
 * A failing case may hold a lone surrogate — that is exactly what some of these tests guard
 * against — so the conversion falls back to a code-unit dump instead of throwing out of the
 * reporter.
 */
template <>
struct StringMaker<std::u16string> {
    static String convert(const std::u16string &value) {
        auto utf8 = std::string{};
        try {
            utf8::utf16to8(value.begin(), value.end(), std::back_inserter(utf8));
        } catch (const utf8::exception &) {
            utf8 = "<invalid UTF-16:";
            for (const auto unit : value) {
                utf8.append(" ").append(std::to_string(static_cast<uint16_t>(unit)));
            }
            utf8.append(">");
        }
        return String(utf8.c_str());
    }
};

}



/**
 * @brief Fills a fresh cursor with text, then drops the history and returns the caret to the origin.
 *
 * The seeding insert is an edit like any other, so the history has to be wiped afterwards for a
 * test to start from a clean undo stack — the same thing OpenFileCommand::loadInto does after
 * loading a file.
 *
 * @param cursor The cursor to seed; must be empty.
 * @param text The text to insert.
 */
inline void seed(Cursor &cursor, const std::u16string_view text) {
    if (!text.empty()) {
        (void) cursor.insert(text);
    }
    cursor.clearHistory();
    cursor.setPosition(0, 0);
    cursor.setModified(false);
}

/**
 * @brief Undoes one step and reports whether anything was undone.
 *
 * The two step helpers are the only place these tests touch the shape of Cursor::undo/redo's
 * return value. Everything else asserts on the buffer text and the caret, which is the behaviour
 * that must survive a change of undo representation.
 *
 * @param cursor The cursor to undo on.
 * @return true when a step was undone, false when the history was exhausted.
 */
inline bool undoStep(Cursor &cursor) {
    return !cursor.undo().empty();
}

/**
 * @brief Redoes one step and reports whether anything was redone.
 *
 * @param cursor The cursor to redo on.
 * @return true when a step was redone, false when there was nothing to replay.
 */
inline bool redoStep(Cursor &cursor) {
    return !cursor.redo().empty();
}

/**
 * @brief Closes the current undo group and puts the caret at the end of its line.
 *
 * A group runs until the next boundary, and a cursor move is what marks one. Appending through
 * this helper therefore produces one undo step per call, which is what the multi-step cases below
 * need.
 *
 * @param cursor The cursor to append with.
 * @param text The text to append at the end of the current line.
 */
inline void appendAsNewGroup(Cursor &cursor, const std::u16string_view text) {
    cursor.moveToEndOfLine();
    (void) cursor.insert(text);
}

/**
 * @brief Types text one character at a time, the way Editor::onTextInput feeds it.
 *
 * A run of single-character inserts with no cursor move between them stays inside one undo group,
 * which is what makes typing undo as a run rather than character by character.
 *
 * @param cursor The cursor to type into.
 * @param text The characters to insert in order.
 */
inline void type(Cursor &cursor, const std::u16string_view text) {
    for (const auto character : text) {
        (void) cursor.insert(std::u16string_view(&character, 1));
    }
}

/**
 * @brief Selects a range by anchoring at one point and moving the caret to the other.
 *
 * @param cursor The cursor to select on.
 * @param lineStart The line the selection is anchored at.
 * @param columnStart The column the selection is anchored at.
 * @param lineEnd The line the caret moves to.
 * @param columnEnd The column the caret moves to.
 */
inline void select(Cursor &cursor, const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) {
    cursor.activateSelection(false);
    cursor.setPosition(lineStart, columnStart);
    cursor.activateSelection(true);
    cursor.setPosition(lineEnd, columnEnd);
}

/**
 * @brief Undoes until the history is spent and reports how many steps that took.
 *
 * @param cursor The cursor to walk back.
 * @return The number of undo steps the history held.
 */
inline uint32_t undoAll(Cursor &cursor) {
    auto steps = 0u;
    while (undoStep(cursor)) {
        ++steps;
    }
    return steps;
}

/**
 * @brief Reports whether a UTF-16 buffer contains a surrogate that lost its partner.
 *
 * Text equality already catches most damage, but a split pair is worth naming: it is the one
 * corruption that makes a buffer unencodable to UTF-8, so saving would throw rather than round-trip.
 *
 * @param text The buffer to scan.
 * @return true when a high surrogate is unfollowed or a low surrogate unpreceded.
 */
inline bool hasLoneSurrogate(const std::u16string_view text) {
    for (auto index = std::size_t{0}; index < text.length(); ++index) {
        const auto is_high = (text[index] & 0xFC00) == 0xD800;
        const auto is_low = (text[index] & 0xFC00) == 0xDC00;
        if (is_high && (index + 1 >= text.length() || (text[index + 1] & 0xFC00) != 0xDC00)) {
            return true;
        }
        if (is_low && (index == 0 || (text[index - 1] & 0xFC00) != 0xD800)) {
            return true;
        }
    }
    return false;
}



#endif //TEST_SUPPORT_H
