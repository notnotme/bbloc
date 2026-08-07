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
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#include <utf8.h>

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


namespace {

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
void seed(Cursor &cursor, const std::u16string_view text) {
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
bool undoStep(Cursor &cursor) {
    return cursor.undo().has_value();
}

/**
 * @brief Redoes one step and reports whether anything was redone.
 *
 * @param cursor The cursor to redo on.
 * @return true when a step was redone, false when there was nothing to replay.
 */
bool redoStep(Cursor &cursor) {
    return cursor.redo().has_value();
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
void appendAsNewGroup(Cursor &cursor, const std::u16string_view text) {
    cursor.moveToEndOfLine();
    (void) cursor.insert(text);
}

}


TEST_CASE("a seeded buffer holds its text and an empty history") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"hello");

    CHECK(cursor.getText() == std::u16string(u"hello"));
    CHECK(cursor.getLineCount() == 1);
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    // Nothing to undo: the seeding edit was wiped with the history
    CHECK_FALSE(undoStep(cursor));
}

TEST_CASE("undo and redo are no-ops on an empty history") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"hello");

    CHECK_FALSE(undoStep(cursor));
    CHECK_FALSE(redoStep(cursor));

    // A refused step must leave the buffer exactly as it was
    CHECK(cursor.getText() == std::u16string(u"hello"));
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("a sequence of edits undoes back to the original text and caret") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    appendAsNewGroup(cursor, u"aaa");
    appendAsNewGroup(cursor, u"bbb");
    appendAsNewGroup(cursor, u"ccc");
    REQUIRE(cursor.getText() == std::u16string(u"aaabbbccc"));

    // Each undo restores the text as it stood when its group opened, and the caret with it
    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaabbb"));
    CHECK(cursor.getColumn() == 6);

    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaa"));
    CHECK(cursor.getColumn() == 3);

    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u""));
    CHECK(cursor.getColumn() == 0);

    // The history is spent, and the buffer stays at the original state
    CHECK_FALSE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u""));
}

TEST_CASE("redo replays the undone edits in order") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    appendAsNewGroup(cursor, u"aaa");
    appendAsNewGroup(cursor, u"bbb");
    appendAsNewGroup(cursor, u"ccc");

    REQUIRE(undoStep(cursor));
    REQUIRE(undoStep(cursor));
    REQUIRE(undoStep(cursor));
    REQUIRE(cursor.getText() == std::u16string(u""));

    REQUIRE(redoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaa"));

    REQUIRE(redoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaabbb"));

    REQUIRE(redoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaabbbccc"));

    // Fully replayed: the caret is back where the last edit left it
    CHECK(cursor.getColumn() == 9);
    CHECK_FALSE(redoStep(cursor));
}

TEST_CASE("an edit after an undo drops the redo stack") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    appendAsNewGroup(cursor, u"aaa");
    appendAsNewGroup(cursor, u"bbb");

    REQUIRE(undoStep(cursor));
    REQUIRE(cursor.getText() == std::u16string(u"aaa"));

    // The branch that was undone is now unreachable
    appendAsNewGroup(cursor, u"ccc");
    REQUIRE(cursor.getText() == std::u16string(u"aaaccc"));

    CHECK_FALSE(redoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaaccc"));

    // The surviving history still walks back through the new branch
    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaa"));
    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u""));
}
