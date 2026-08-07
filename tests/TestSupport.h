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

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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



/**
 * @brief The reference model a text buffer is checked against: one string per line.
 *
 * Separators are implied exactly as the buffer implies them — the model never holds a '\n' — so
 * every offset the buffer reports has to be rebuilt from the line lengths, which is the arithmetic
 * a stale line offset breaks.
 */
using BufferModel = std::vector<std::u16string>;

/**
 * @brief Splits a flat text into the lines a buffer holding it must expose.
 *
 * @param text The text to split.
 * @return One entry per line; a single empty line for an empty text.
 */
inline BufferModel splitLines(const std::u16string_view text) {
    auto model = BufferModel{std::u16string{}};
    for (const auto unit : text) {
        if (unit == u'\n') {
            model.emplace_back();
        } else {
            model.back().push_back(unit);
        }
    }
    return model;
}

/**
 * @brief Joins a model back into one flat text, one separator between lines.
 *
 * Edits are applied to the model through this flat form: string surgery on a single string is
 * obviously right, which is the whole point of having a model to compare against.
 *
 * @param model The model to join.
 * @return The text the model stands for.
 */
inline std::u16string joinLines(const BufferModel &model) {
    auto text = std::u16string{};
    for (auto line = std::size_t{0}; line < model.size(); ++line) {
        if (line > 0) {
            text.push_back(u'\n');
        }
        text.append(model[line]);
    }
    return text;
}

/**
 * @brief Orders a range so it runs forward, the way the buffer normalises a backwards erase.
 *
 * @param lineStart The line the range starts at, swapped when the range runs backwards.
 * @param columnStart The column the range starts at, swapped with the range.
 * @param lineEnd The line the range ends at.
 * @param columnEnd The column the range ends at.
 */
inline void normalizeRange(uint32_t &lineStart, uint32_t &columnStart, uint32_t &lineEnd, uint32_t &columnEnd) {
    if (lineStart > lineEnd || (lineStart == lineEnd && columnStart > columnEnd)) {
        std::swap(lineStart, lineEnd);
        std::swap(columnStart, columnEnd);
    }
}

/**
 * @brief Returns the character offset of a position in the model's flat text.
 *
 * Counts the characters of every preceding line plus one separator per preceding line, which is
 * what makes it independent of how the buffer lays its lines out.
 *
 * @param model The model to measure.
 * @param line The line of the position.
 * @param column The column of the position.
 * @return The number of characters before that position in the flat text.
 */
inline uint32_t modelOffset(const BufferModel &model, const uint32_t line, const uint32_t column) {
    auto offset = line + column;                // one separator per preceding line
    for (auto index = uint32_t{0}; index < line; ++index) {
        offset += static_cast<uint32_t>(model[index].length());
    }
    return offset;
}

/**
 * @brief Returns the byte offset of a position, in the units the buffer reports.
 *
 * @param model The model to measure.
 * @param line The line of the position.
 * @param column The column of the position.
 * @return The byte offset of that position.
 */
inline uint32_t modelByteOffset(const BufferModel &model, const uint32_t line, const uint32_t column) {
    return static_cast<uint32_t>(modelOffset(model, line, column) * sizeof(char16_t));
}

/**
 * @brief Applies an insert to the model.
 *
 * @param model The model to edit.
 * @param line The line to insert at.
 * @param column The column to insert at.
 * @param characters The characters to insert, separators included.
 */
inline void modelInsert(BufferModel &model, const uint32_t line, const uint32_t column, const std::u16string_view characters) {
    auto text = joinLines(model);
    text.insert(modelOffset(model, line, column), characters);
    model = splitLines(text);
}

/**
 * @brief Applies an erase to the model, accepting a backwards range as the buffer does.
 *
 * @param model The model to edit.
 * @param lineStart The line the erased range starts at.
 * @param columnStart The column the erased range starts at.
 * @param lineEnd The line the erased range ends at.
 * @param columnEnd The column the erased range ends at.
 */
inline void modelErase(BufferModel &model, uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) {
    normalizeRange(lineStart, columnStart, lineEnd, columnEnd);

    const auto start = modelOffset(model, lineStart, columnStart);
    const auto end = modelOffset(model, lineEnd, columnEnd);

    auto text = joinLines(model);
    text.erase(start, end - start);
    model = splitLines(text);
}

/**
 * @brief Returns the weighted length of the model's longest line.
 *
 * @param model The model to measure.
 * @param tabWeight The number of character widths a tab occupies; values below one weigh one.
 * @return The weighted length of the longest line.
 */
inline uint32_t modelLongestLineLength(const BufferModel &model, const uint32_t tabWeight) {
    const auto weight = std::max(tabWeight, 1u);

    auto longest = uint32_t{0};
    for (const auto &line : model) {
        const auto tab_count = static_cast<uint32_t>(std::count(line.begin(), line.end(), u'\t'));
        longest = std::max(longest, static_cast<uint32_t>(line.length()) + tab_count * (weight - 1));
    }
    return longest;
}

/**
 * @brief Returns the number of tabs on one line of the model.
 *
 * @param model The model to measure.
 * @param line The line to count the tabs of.
 * @return The tab count of that line.
 */
inline uint32_t modelTabCount(const BufferModel &model, const uint32_t line) {
    return static_cast<uint32_t>(std::count(model[line].begin(), model[line].end(), u'\t'));
}

/**
 * @brief Reports whether one position comes at or before another.
 *
 * @param first The position expected to come first.
 * @param second The position expected to come second.
 * @return true when first is at or before second in (line, column) order.
 */
inline bool positionIsOrdered(const BufferEdit::Position &first, const BufferEdit::Position &second) {
    return first.line < second.line || (first.line == second.line && first.column <= second.column);
}

/**
 * @brief Asserts a BufferEdit is self-consistent, whatever the edit was.
 *
 * The struct is the highlighter's entire input: an edit whose start sits after one of its ends, or
 * whose bytes disagree with that order, makes tree-sitter re-parse a range that does not exist.
 *
 * @param edit The edit to check.
 */
inline void checkEditIsConsistent(const BufferEdit &edit) {
    CHECK(positionIsOrdered(edit.start, edit.old_end));
    CHECK(positionIsOrdered(edit.start, edit.new_end));
    CHECK(edit.start_byte <= edit.old_end_byte);
    CHECK(edit.start_byte <= edit.new_end_byte);
}

/**
 * @brief Asserts a buffer agrees with its model on every observable it has.
 *
 * The weights are queried as 1, then 4, then 1 again: the tracker only recomputes its maximum when
 * the weight changes, so the leading query at the weight the previous call left behind is the one
 * that reads the incrementally maintained value, and the trailing one puts the weight back so the
 * next call still does. Checking a single weight would never exercise the rescan, checking two in
 * any other order would never exercise anything else.
 *
 * @param buffer The buffer under test.
 * @param model The model it is expected to match.
 */
inline void checkMatches(const TextBuffer &buffer, const BufferModel &model) {
    REQUIRE(buffer.getStringCount() == model.size());

    const auto last_line = static_cast<uint32_t>(model.size()) - 1;
    for (auto line = uint32_t{0}; line <= last_line; ++line) {
        CAPTURE(line);
        const auto length = static_cast<uint32_t>(model[line].length());

        CHECK(std::u16string(buffer.getString(line)) == model[line]);
        CHECK(buffer.getLineTabCount(line) == modelTabCount(model, line));

        // Both ends of the line: the start carries the offsets of everything above it, the end adds
        // the line's own length on top
        CHECK(buffer.getByteOffset(line, 0) == modelByteOffset(model, line, 0));
        CHECK(buffer.getByteOffset(line, length) == modelByteOffset(model, line, length));

        // Ranges spanning the line, and the separator that follows it
        CHECK(buffer.getByteCount(line, 0, line, length) == modelByteOffset(model, line, length) - modelByteOffset(model, line, 0));
        if (line < last_line) {
            CHECK(buffer.getByteCount(line, length, line + 1, 0) == sizeof(char16_t));
        }
    }

    const auto last_column = static_cast<uint32_t>(model[last_line].length());
    const auto whole = modelByteOffset(model, last_line, last_column);
    CHECK(buffer.getByteCount(0, 0, last_line, last_column) == whole);
    CHECK(buffer.getByteCount(last_line, last_column, 0, 0) == whole);       // a backwards range measures the same
    CHECK(buffer.getByteCount(0, 0, 0, 0) == 0);

    CHECK(buffer.getLongestLineLength(1) == modelLongestLineLength(model, 1));
    CHECK(buffer.getLongestLineLength(4) == modelLongestLineLength(model, 4));
    CHECK(buffer.getLongestLineLength(1) == modelLongestLineLength(model, 1));
}

/**
 * @brief Fills a fresh buffer with text and returns the model standing beside it.
 *
 * @param buffer The buffer to seed; must be empty.
 * @param text The text to insert at the origin.
 * @return The model of the seeded buffer.
 */
inline BufferModel seedBuffer(LineBuffer &buffer, const std::u16string_view text) {
    auto model = BufferModel{std::u16string{}};
    if (!text.empty()) {
        (void) buffer.insert(0, 0, text);
        model = splitLines(text);
    }

    checkMatches(buffer, model);
    return model;
}

/**
 * @brief Inserts into the buffer and the model at once, then checks the edit and the agreement.
 *
 * The edit is spelled out entirely from the model taken before and after the insert, so a case only
 * has to say where it inserted what.
 *
 * @param buffer The buffer to edit.
 * @param model The model to apply the same edit to.
 * @param line The line to insert at.
 * @param column The column to insert at.
 * @param characters The characters to insert, separators included.
 * @return The edit the buffer reported.
 */
inline BufferEdit applyInsert(LineBuffer &buffer, BufferModel &model, const uint32_t line, const uint32_t column, const std::u16string_view characters) {
    const auto start_byte = modelByteOffset(model, line, column);

    const auto edit = buffer.insert(line, column, characters);
    modelInsert(model, line, column, characters);

    checkEditIsConsistent(edit);

    // An insert replaces nothing, so both of its old ends stay at the position it was made at
    CHECK(edit.start.line == line);
    CHECK(edit.start.column == column);
    CHECK(edit.old_end.line == line);
    CHECK(edit.old_end.column == column);
    CHECK(edit.start_byte == start_byte);
    CHECK(edit.old_end_byte == start_byte);

    // The new end is wherever walking the inserted text from the start lands
    const auto new_end = advancePosition(edit.start, characters);
    CHECK(edit.new_end.line == new_end.line);
    CHECK(edit.new_end.column == new_end.column);
    CHECK(edit.new_end_byte == modelByteOffset(model, new_end.line, new_end.column));

    checkMatches(buffer, model);
    return edit;
}

/**
 * @brief Erases from the buffer and the model at once, then checks the edit and the agreement.
 *
 * @param buffer The buffer to edit.
 * @param model The model to apply the same erase to.
 * @param lineStart The line the erased range starts at.
 * @param columnStart The column the erased range starts at.
 * @param lineEnd The line the erased range ends at.
 * @param columnEnd The column the erased range ends at.
 * @return The edit the buffer reported.
 */
inline BufferEdit applyErase(LineBuffer &buffer, BufferModel &model, const uint32_t lineStart, const uint32_t columnStart, const uint32_t lineEnd, const uint32_t columnEnd) {
    auto first_line = lineStart;
    auto first_column = columnStart;
    auto last_line = lineEnd;
    auto last_column = columnEnd;
    normalizeRange(first_line, first_column, last_line, last_column);

    const auto start_byte = modelByteOffset(model, first_line, first_column);
    const auto old_end_byte = modelByteOffset(model, last_line, last_column);

    const auto edit = buffer.erase(lineStart, columnStart, lineEnd, columnEnd);
    modelErase(model, lineStart, columnStart, lineEnd, columnEnd);

    checkEditIsConsistent(edit);

    // An erase reports the range it took, and collapses it onto its start
    CHECK(edit.start.line == first_line);
    CHECK(edit.start.column == first_column);
    CHECK(edit.old_end.line == last_line);
    CHECK(edit.old_end.column == last_column);
    CHECK(edit.new_end.line == first_line);
    CHECK(edit.new_end.column == first_column);
    CHECK(edit.start_byte == start_byte);
    CHECK(edit.old_end_byte == old_end_byte);
    CHECK(edit.new_end_byte == start_byte);
    CHECK(edit.new_end_byte == modelByteOffset(model, first_line, first_column));

    checkMatches(buffer, model);
    return edit;
}



#endif //TEST_SUPPORT_H
