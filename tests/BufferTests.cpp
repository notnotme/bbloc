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
#include "TestSupport.h"


TEST_CASE("a fresh buffer is one empty line") {
    auto buffer = LineBuffer();

    // The single line exists before anything is inserted, and it is already the current one
    const auto model = seedBuffer(buffer, u"");
    CHECK(model.size() == 1);
    CHECK(buffer.getStringCount() == 1);
    CHECK(buffer.getByteOffset(0, 0) == 0);
    CHECK(buffer.getLongestLineLength(1) == 0);
}

TEST_CASE("a single-line insert lands in the line it was made on") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // The seed left line 2 detached, so this insert takes the fast path that never touches m_buffer
    (void) applyInsert(buffer, model, 2, 2, u"XY");
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u"thXYree"));

    // And this one takes the slow path, on a line the buffer really holds
    (void) applyInsert(buffer, model, 0, 3, u"!");
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"one!"));
}

TEST_CASE("a multi-line insert splits the line it lands in") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // Two newlines in one splice: the tail of the touched line has to come back out at the end of
    // the last inserted segment, and every line below has to move by the whole insertion
    (void) applyInsert(buffer, model, 1, 1, u"a\nb\nc");
    CHECK(buffer.getStringCount() == 5);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"ta"));
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u"b"));
    CHECK(std::u16string(buffer.getString(3)) == std::u16string(u"cwo"));
    CHECK(std::u16string(buffer.getString(4)) == std::u16string(u"three"));
}

TEST_CASE("an insert ending in a separator leaves an empty line behind") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo");

    // The trailing segment is empty, so the line the edit ends on holds nothing but the remainder
    (void) applyInsert(buffer, model, 0, 3, u"!\n");
    CHECK(buffer.getStringCount() == 3);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u""));
}

TEST_CASE("a bare separator splits the current line in two") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo");

    // A lone newline on the detached line has its own path, which commits the head only
    (void) applyInsert(buffer, model, 1, 1, u"\n");
    CHECK(buffer.getStringCount() == 3);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"t"));
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u"wo"));

    // The split line is the new current one; editing it must not disturb what sits above
    (void) applyInsert(buffer, model, 2, 0, u"T");
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"one"));
}

TEST_CASE("erasing inside one line keeps the rest of the buffer in place") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    (void) applyErase(buffer, model, 2, 1, 2, 4);
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u"te"));

    // The same erase on a line the buffer holds, rather than on the detached one
    (void) applyErase(buffer, model, 0, 0, 0, 2);
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"e"));
}

TEST_CASE("erasing across lines joins the ends of the range") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree\nfour");

    (void) applyErase(buffer, model, 0, 1, 2, 2);
    CHECK(buffer.getStringCount() == 2);
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"oree"));
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"four"));
}

TEST_CASE("erasing a whole line takes its separator with it") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree\nfour");

    (void) applyErase(buffer, model, 1, 0, 2, 0);
    CHECK(buffer.getStringCount() == 3);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"three"));
}

TEST_CASE("erasing a backwards range erases the same characters") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // What a selection dragged upwards hands over
    (void) applyErase(buffer, model, 2, 2, 0, 1);
    CHECK(buffer.getStringCount() == 1);
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"oree"));
}

TEST_CASE("erasing everything leaves one empty line") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    (void) applyErase(buffer, model, 0, 0, 2, 5);
    CHECK(buffer.getStringCount() == 1);
    CHECK(buffer.getByteOffset(0, 0) == 0);

    // The emptied buffer is still an editable one, not a wedged one
    (void) applyInsert(buffer, model, 0, 0, u"again");
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"again"));
}

TEST_CASE("inserting at the very end of the buffer extends the last line") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo");

    // On the detached last line first, then on the same spot with another line detached, so both
    // paths compute the end of the buffer
    (void) applyInsert(buffer, model, 1, 3, u"!");
    (void) applyInsert(buffer, model, 0, 0, u"@");
    (void) applyInsert(buffer, model, 1, 4, u"?");
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"two!?"));

    // A separator at the very end opens a line with nothing after it
    (void) applyInsert(buffer, model, 1, 5, u"\n");
    CHECK(buffer.getStringCount() == 3);
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u""));
}

TEST_CASE("erasing at the very end of the buffer shortens the last line") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    (void) applyErase(buffer, model, 2, 4, 2, 5);
    CHECK(std::u16string(buffer.getString(2)) == std::u16string(u"thre"));

    // Joining the last line into the one above it, with that upper line the detached one: what a
    // delete pressed at the end of a line does
    (void) applyInsert(buffer, model, 1, 1, u"X");
    (void) applyErase(buffer, model, 1, 4, 2, 0);
    CHECK(buffer.getStringCount() == 2);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"tXwothre"));
}

TEST_CASE("joining a line into the one above it works from either side") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // The seed leaves the lower line detached, which is the state a backspace at column 0 finds:
    // the upper line's head has to be pulled in front of the detached one
    (void) applyErase(buffer, model, 1, 3, 2, 0);
    CHECK(buffer.getStringCount() == 2);
    CHECK(std::u16string(buffer.getString(1)) == std::u16string(u"twothree"));

    // The join is only a merge of two neighbours, so the line above them must not have moved
    CHECK(std::u16string(buffer.getString(0)) == std::u16string(u"one"));
}

TEST_CASE("an empty insert describes the position it was asked about") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // Byte offsets left at zero would agree with nothing and make the re-parse dirty the whole
    // start of the tree, so the degenerate edit has to carry the real position
    const auto edit = applyInsert(buffer, model, 2, 3, u"");
    CHECK(edit.start_byte == 22);
    CHECK(edit.old_end_byte == 22);
    CHECK(edit.new_end_byte == 22);

    // Same position, but reached with a line above it detached: the offset has to be corrected by
    // the detached line's length, which the fast return still has to do
    (void) applyInsert(buffer, model, 0, 0, u"@");
    const auto detached = applyInsert(buffer, model, 2, 3, u"");
    CHECK(detached.start_byte == 24);
}

TEST_CASE("an empty erase range describes the position it was asked about") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    (void) applyInsert(buffer, model, 0, 0, u"@");
    const auto edit = applyErase(buffer, model, 2, 3, 2, 3);
    CHECK(edit.start_byte == 24);
    CHECK(edit.old_end_byte == 24);
    CHECK(edit.new_end_byte == 24);
}

TEST_CASE("clear reports the whole buffer as erased and leaves one empty line") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"one\ntwo\nthree");

    // Cleared from a state with a detached line, since that is the only state the editor is ever in
    (void) applyInsert(buffer, model, 0, 1, u"!");
    const auto edit = buffer.clear();
    model = BufferModel{std::u16string{}};

    checkEditIsConsistent(edit);
    CHECK(edit.start_byte == 0);
    CHECK(edit.new_end_byte == 0);
    CHECK(edit.old_end.line == 2);
    CHECK(edit.old_end.column == 5);
    CHECK(edit.old_end_byte == 28);          // "o!ne\ntwo\nthree", separators included
    checkMatches(buffer, model);

    // Nothing of the cleared text is left behind, not even in the metrics
    CHECK(buffer.getLongestLineLength(1) == 0);
    (void) applyInsert(buffer, model, 0, 0, u"fresh");
}

TEST_CASE("edits alternating between distant lines keep every observable straight") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"zero\none\ntwo\nthree\nfour");

    // Each hop commits the line that was detached and detaches another: the offsets of everything
    // between them are rewritten every time, and a single stale one shows up in the next check
    (void) applyInsert(buffer, model, 3, 2, u"AAA");
    (void) applyInsert(buffer, model, 0, 0, u"B");
    (void) applyInsert(buffer, model, 3, 0, u"C");
    (void) applyErase(buffer, model, 0, 0, 0, 1);
    (void) applyInsert(buffer, model, 4, 4, u"D");
    (void) applyErase(buffer, model, 3, 1, 3, 3);
    (void) applyInsert(buffer, model, 1, 3, u"E");

    // A multi-line insert far from the detached line, then a hop straight back onto it
    (void) applyInsert(buffer, model, 4, 2, u"x\ny");
    (void) applyInsert(buffer, model, 0, 0, u"F");
    (void) applyErase(buffer, model, 4, 0, 5, 1);
    (void) applyInsert(buffer, model, 2, 1, u"G");

    CHECK(joinLines(model) == std::u16string(u"Fzero\noneE\ntGwo\nCAAAree\nurD"));
}

TEST_CASE("byte offsets stay right while a middle line is detached") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"aaa\nbbbb\nccccc\nd");

    // Line 1 is detached from here on: its characters are absent from the main buffer, so the lines
    // below it only report the right offsets if the detached length is added back
    (void) applyInsert(buffer, model, 1, 2, u"X");
    REQUIRE(std::u16string(buffer.getString(1)) == std::u16string(u"bbXbb"));

    CHECK(buffer.getByteOffset(0, 0) == 0);
    CHECK(buffer.getByteOffset(1, 0) == 8);       // "aaa" plus one separator
    CHECK(buffer.getByteOffset(2, 0) == 20);      // "aaa" and "bbXbb" plus two separators
    CHECK(buffer.getByteOffset(3, 0) == 32);
    CHECK(buffer.getByteOffset(3, 1) == 34);

    CHECK(buffer.getByteCount(1, 0, 3, 1) == 26);
    CHECK(buffer.getByteCount(2, 1, 2, 4) == 6);
}

TEST_CASE("the longest line follows a line as it grows") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"aa\nbbbb\ncc");
    REQUIRE(buffer.getLongestLineLength(1) == 4);

    // A line growing past the maximum takes it over without any rescan
    (void) applyInsert(buffer, model, 0, 2, u"aaaaa");
    CHECK(buffer.getLongestLineLength(1) == 7);

    // And a line growing but staying below it leaves it alone
    (void) applyInsert(buffer, model, 2, 2, u"c");
    CHECK(buffer.getLongestLineLength(1) == 7);

    // A new line spliced in above the longest one moves it down, which the metrics have to follow
    (void) applyInsert(buffer, model, 0, 0, u"\n");
    CHECK(buffer.getLongestLineLength(1) == 7);
}

TEST_CASE("the longest line is found again when the longest line shrinks") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"aa\nbbbbbb\ncccc");
    REQUIRE(buffer.getLongestLineLength(1) == 6);

    // The tracked maximum was on the line that just lost characters, so it is now a stale value no
    // incremental update can correct: only a rescan of the metrics finds the runner-up
    (void) applyErase(buffer, model, 1, 1, 1, 5);
    CHECK(buffer.getLongestLineLength(1) == 4);

    // Same thing when the longest line disappears entirely
    (void) applyInsert(buffer, model, 0, 2, u"aaaaaaa");
    REQUIRE(buffer.getLongestLineLength(1) == 9);
    (void) applyErase(buffer, model, 0, 0, 1, 0);
    CHECK(buffer.getLongestLineLength(1) == 4);
}

TEST_CASE("tabs weigh the tab weight in the longest line") {
    auto buffer = LineBuffer();
    auto model = seedBuffer(buffer, u"\t\tx\nyyyyy");

    CHECK(buffer.getLineTabCount(0) == 2);
    CHECK(buffer.getLineTabCount(1) == 0);

    // Which line is the longest depends on the weight, so changing it has to force a rescan
    CHECK(buffer.getLongestLineLength(1) == 5);
    CHECK(buffer.getLongestLineLength(4) == 9);
    CHECK(buffer.getLongestLineLength(8) == 17);
    CHECK(buffer.getLongestLineLength(1) == 5);

    // A weight of zero is not a zero-width tab, it is one character like everything else
    CHECK(buffer.getLongestLineLength(0) == 5);

    // Tabs added and removed keep the count and the weighted maximum in step
    (void) applyInsert(buffer, model, 1, 0, u"\t");
    CHECK(buffer.getLineTabCount(1) == 1);
    CHECK(buffer.getLongestLineLength(4) == 9);

    (void) applyErase(buffer, model, 0, 0, 0, 2);
    CHECK(buffer.getLineTabCount(0) == 0);
    CHECK(buffer.getLongestLineLength(4) == 9);
}

TEST_CASE("advancePosition leaves the position untouched for an empty text") {
    const auto start = BufferEdit::Position{.line = 3, .column = 7};
    const auto end = advancePosition(start, u"");

    CHECK(end.line == 3);
    CHECK(end.column == 7);
}

TEST_CASE("advancePosition walks the column over a text with no separator") {
    const auto start = BufferEdit::Position{.line = 3, .column = 7};
    const auto end = advancePosition(start, u"abcd");

    CHECK(end.line == 3);
    CHECK(end.column == 11);
}

TEST_CASE("advancePosition lands on column zero after a trailing separator") {
    const auto start = BufferEdit::Position{.line = 3, .column = 7};
    const auto end = advancePosition(start, u"ab\n");

    // The column the text started at is spent: the next character would be the first of a new line
    CHECK(end.line == 4);
    CHECK(end.column == 0);
}

TEST_CASE("advancePosition counts every line of a multi-line text") {
    const auto start = BufferEdit::Position{.line = 3, .column = 7};
    const auto end = advancePosition(start, u"ab\ncd\nefg");

    // Only the last segment contributes to the column; the ones before it are behind separators
    CHECK(end.line == 5);
    CHECK(end.column == 3);

    // A text of separators only walks straight down, column by column of nothing
    const auto empty_lines = advancePosition(start, u"\n\n");
    CHECK(empty_lines.line == 5);
    CHECK(empty_lines.column == 0);
}
