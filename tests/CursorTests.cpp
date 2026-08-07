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
#include <stdexcept>

#include "TestSupport.h"


TEST_CASE("a vertical move onto a shorter line lands at that line's end") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"hello world\nab\nlong enough line");

    cursor.setPosition(0, 8);
    cursor.moveDown();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 2);

    // The clamp is permanent here: the remembered column that would put the caret back at 8 lives
    // in CursorContext, not in the cursor, so moving on keeps the clamped value
    cursor.moveDown();
    CHECK(cursor.getLine() == 2);
    CHECK(cursor.getColumn() == 2);

    cursor.setPosition(2, 9);
    cursor.moveUp();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 2);

    cursor.moveUp();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 2);
}

TEST_CASE("a vertical move at a buffer edge slides to the line edge") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"abc\ndefgh");

    // There is no line above, so up means the start of the file's first line
    cursor.setPosition(0, 2);
    cursor.moveUp();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    // And no line below, so down means the end of the last one
    cursor.setPosition(1, 2);
    cursor.moveDown();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 5);
}

TEST_CASE("a horizontal move steps a non-BMP character in one go") {
    const auto grin = std::u16string(u"\U0001F600");
    REQUIRE(grin.length() == 2);

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, std::u16string(u"a") + grin + u"b");

    // Right over the pair: one move, two code units, never a stop between them
    cursor.setPosition(0, 1);
    cursor.moveRight();
    CHECK(cursor.getColumn() == 3);
    cursor.moveRight();
    CHECK(cursor.getColumn() == 4);

    cursor.moveLeft();
    CHECK(cursor.getColumn() == 3);
    cursor.moveLeft();
    CHECK(cursor.getColumn() == 1);
    cursor.moveLeft();
    CHECK(cursor.getColumn() == 0);

    // Every column the caret stopped at cuts the line into two encodable halves
    for (const auto column : {0u, 1u, 3u, 4u}) {
        CAPTURE(column);
        CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(0, column)));
        CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(column)));
    }
}

TEST_CASE("a vertical move landing inside a surrogate pair is pulled off it") {
    const auto grin = std::u16string(u"\U0001F600");

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    // The middle line puts the pair at columns 1 and 2, so column 2 of the lines around it splits it
    seed(cursor, std::u16string(u"abcd\na") + grin + u"b\nefgh");

    cursor.setPosition(0, 2);
    cursor.moveDown();
    REQUIRE(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);
    CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(0, cursor.getColumn())));
    CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(cursor.getColumn())));

    cursor.setPosition(2, 2);
    cursor.moveUp();
    REQUIRE(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);
    CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(0, cursor.getColumn())));
    CHECK_FALSE(hasLoneSurrogate(cursor.getString().substr(cursor.getColumn())));
}

TEST_CASE("a horizontal move at either end of the buffer stays put") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"ab\ncd");

    cursor.setPosition(0, 0);
    cursor.moveLeft();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    cursor.moveToEndOfFile();
    REQUIRE(cursor.getLine() == 1);
    REQUIRE(cursor.getColumn() == 2);
    cursor.moveRight();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 2);
}

TEST_CASE("a horizontal move wraps between lines") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"abc\nde");

    // Left off column 0 lands past the last character of the line above, not on it
    cursor.setPosition(1, 0);
    cursor.moveLeft();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 3);

    cursor.moveRight();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("the line and file boundary moves land where they say") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    cursor.setPosition(1, 1);
    cursor.moveToEndOfLine();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 3);

    cursor.moveToStartOfLine();
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 0);

    cursor.moveToEndOfFile();
    CHECK(cursor.getLine() == 2);
    CHECK(cursor.getColumn() == 5);

    cursor.moveToStartOfFile();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("the boundary moves are no-ops on an empty buffer") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    // The single empty line is both ends of the file, and every move has to survive it
    cursor.moveToEndOfFile();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    cursor.moveToEndOfLine();
    cursor.moveRight();
    cursor.moveDown();
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("a page move steps by its line count and clamps at both ends") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"l0\nl1\nl2\nl3\nl4\nl5");
    REQUIRE(cursor.getLineCount() == 6);

    cursor.setPosition(0, 0);
    cursor.pageDown(2);
    CHECK(cursor.getLine() == 2);
    cursor.pageDown(2);
    CHECK(cursor.getLine() == 4);

    // 4 + 2 is past the last line, so the move stops on it rather than running off
    cursor.pageDown(2);
    CHECK(cursor.getLine() == 5);

    cursor.pageUp(2);
    CHECK(cursor.getLine() == 3);
    cursor.pageUp(2);
    CHECK(cursor.getLine() == 1);
    cursor.pageUp(2);
    CHECK(cursor.getLine() == 0);

    // A page taller than the buffer is the ordinary case on a short file
    cursor.pageDown(100);
    CHECK(cursor.getLine() == 5);
    cursor.pageUp(100);
    CHECK(cursor.getLine() == 0);
}

TEST_CASE("a page move onto a shorter line clamps the column") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"abcdef\nx\nabcdef");

    cursor.setPosition(0, 5);
    cursor.pageDown(1);
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);

    cursor.setPosition(2, 5);
    cursor.pageUp(1);
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);
}

TEST_CASE("a page move landing inside a surrogate pair is pulled off it") {
    const auto grin = std::u16string(u"\U0001F600");

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, std::u16string(u"abcd\na") + grin + u"b\nefgh");

    cursor.setPosition(0, 2);
    cursor.pageDown(1);
    REQUIRE(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);

    cursor.setPosition(2, 2);
    cursor.pageUp(1);
    REQUIRE(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 1);
}

TEST_CASE("setPosition refuses coordinates outside the buffer") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    cursor.setPosition(1, 2);

    CHECK_THROWS_AS(cursor.setPosition(3, 0), std::runtime_error);
    CHECK_THROWS_AS(cursor.setPosition(0, 4), std::runtime_error);

    // A refused move must not have half-applied itself
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 2);

    // The far end of the buffer is inside it: the column may equal the line length
    cursor.setPosition(2, 5);
    CHECK(cursor.getLine() == 2);
    CHECK(cursor.getColumn() == 5);
}

TEST_CASE("setPosition snaps a column landing inside a surrogate pair") {
    const auto grin = std::u16string(u"\U0001F600");

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, std::u16string(u"a") + grin + u"b");

    cursor.setPosition(0, 2);
    CHECK(cursor.getColumn() == 1);

    // The columns on either side of the pair are boundaries and must be left alone
    cursor.setPosition(0, 1);
    CHECK(cursor.getColumn() == 1);
    cursor.setPosition(0, 3);
    CHECK(cursor.getColumn() == 3);
}

TEST_CASE("a selection anchored after the caret still runs forward") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    // Anchored on the last line, caret dragged up to the first one
    select(cursor, 2, 2, 0, 1);
    const auto range = cursor.getSelectedRange();
    REQUIRE(range.has_value());
    CHECK(range->line_start == 0);
    CHECK(range->column_start == 1);
    CHECK(range->line_end == 2);
    CHECK(range->column_end == 2);

    // Backwards within one line only swaps the columns
    select(cursor, 1, 3, 1, 1);
    const auto same_line = cursor.getSelectedRange();
    REQUIRE(same_line.has_value());
    CHECK(same_line->line_start == 1);
    CHECK(same_line->column_start == 1);
    CHECK(same_line->line_end == 1);
    CHECK(same_line->column_end == 3);

    const auto text = cursor.getSelectedText();
    REQUIRE(text.has_value());
    REQUIRE(text->size() == 1);
    CHECK(std::u16string(text->at(0)) == std::u16string(u"wo"));
}

TEST_CASE("a degenerate selection reports no range") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    // Armed but never dragged: the anchor sits on the caret, so there is nothing between them
    select(cursor, 1, 2, 1, 2);
    CHECK_FALSE(cursor.getSelectedRange().has_value());
    CHECK_FALSE(cursor.getSelectedText().has_value());

    cursor.activateSelection(false);
    CHECK_FALSE(cursor.getSelectedRange().has_value());
    CHECK_FALSE(cursor.getSelectedText().has_value());
}

TEST_CASE("a single-line selection yields one string") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    select(cursor, 1, 0, 1, 3);
    const auto whole = cursor.getSelectedText();
    REQUIRE(whole.has_value());
    REQUIRE(whole->size() == 1);
    CHECK(std::u16string(whole->at(0)) == std::u16string(u"two"));

    select(cursor, 0, 1, 0, 3);
    const auto partial = cursor.getSelectedText();
    REQUIRE(partial.has_value());
    REQUIRE(partial->size() == 1);
    CHECK(std::u16string(partial->at(0)) == std::u16string(u"ne"));
}

TEST_CASE("a multi-line selection yields one string per line") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree\nfour");

    // The separators are implied: only the first and last lines are cut, the ones between come whole
    select(cursor, 0, 1, 3, 2);
    const auto spanning = cursor.getSelectedText();
    REQUIRE(spanning.has_value());
    REQUIRE(spanning->size() == 4);
    CHECK(std::u16string(spanning->at(0)) == std::u16string(u"ne"));
    CHECK(std::u16string(spanning->at(1)) == std::u16string(u"two"));
    CHECK(std::u16string(spanning->at(2)) == std::u16string(u"three"));
    CHECK(std::u16string(spanning->at(3)) == std::u16string(u"fo"));

    // Two adjacent lines have no whole line between them, so both entries are cuts
    select(cursor, 1, 1, 2, 2);
    const auto adjacent = cursor.getSelectedText();
    REQUIRE(adjacent.has_value());
    REQUIRE(adjacent->size() == 2);
    CHECK(std::u16string(adjacent->at(0)) == std::u16string(u"wo"));
    CHECK(std::u16string(adjacent->at(1)) == std::u16string(u"th"));
}

TEST_CASE("a selection over a non-BMP character keeps both of its code units") {
    const auto grin = std::u16string(u"\U0001F600");

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, std::u16string(u"a") + grin + u"b");

    // Anchoring and ending inside the pair: both ends snap out of it, so the emoji comes whole
    select(cursor, 0, 2, 0, 4);
    const auto text = cursor.getSelectedText();
    REQUIRE(text.has_value());
    REQUIRE(text->size() == 1);
    CHECK(std::u16string(text->at(0)) == grin + u"b");
    CHECK_FALSE(hasLoneSurrogate(text->at(0)));
}

TEST_CASE("the longest line is reported at the queried tab weight") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"ab\n\t\tcd\nefghij");

    // Which line is longest depends on the weight: the plain line wins at 1, the tabbed one at 4
    CHECK(cursor.getLongestLineLength(1) == 6);
    CHECK(cursor.getLongestLineLength(4) == 10);
    CHECK(cursor.getLongestLineLength(8) == 18);
    CHECK(cursor.getLongestLineLength(1) == 6);

    // A tab occupying no column at all is not a thing, so a weight below one counts as one
    CHECK(cursor.getLongestLineLength(0) == 6);

    CHECK(cursor.getLineTabCount(0) == 0);
    CHECK(cursor.getLineTabCount(1) == 2);
    CHECK(cursor.getLineTabCount(2) == 0);
}
