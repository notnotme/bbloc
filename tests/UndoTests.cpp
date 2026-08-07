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

TEST_CASE("a typed run is a single undo step") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    type(cursor, u"hello world");
    REQUIRE(cursor.getText() == std::u16string(u"hello world"));

    CHECK(undoAll(cursor) == 1);
    CHECK(cursor.getText() == std::u16string(u""));
}

TEST_CASE("a caret move splits a typed run") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    type(cursor, u"abc");
    cursor.moveLeft();
    type(cursor, u"d");
    REQUIRE(cursor.getText() == std::u16string(u"abdc"));

    // The move closed the first run, so the two typed spans undo separately
    CHECK(undoAll(cursor) == 2);
    CHECK(cursor.getText() == std::u16string(u""));
}

TEST_CASE("a new line closes its group rather than opening one") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    type(cursor, u"abc");
    (void) cursor.newLine();
    type(cursor, u"def");
    REQUIRE(cursor.getText() == std::u16string(u"abc\ndef"));

    // The line break belongs to the run it ends, so the first undo drops "def"
    // and leaves the empty second line behind
    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"abc\n"));
    CHECK(cursor.getLine() == 1);
    CHECK(cursor.getColumn() == 0);

    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u""));
    CHECK_FALSE(undoStep(cursor));
}

TEST_CASE("a multi-line insert is a single undo step") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    // One insert carrying newlines, the shape PasteTextCommand produces
    (void) cursor.insert(u"one\ntwo\nthree");
    REQUIRE(cursor.getText() == std::u16string(u"one\ntwo\nthree"));
    REQUIRE(cursor.getLineCount() == 3);

    CHECK(undoAll(cursor) == 1);
    CHECK(cursor.getText() == std::u16string(u""));
    CHECK(cursor.getLineCount() == 1);
}

TEST_CASE("a multi-line selection erase is a single undo step") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"one\ntwo\nthree");

    select(cursor, 0, 1, 2, 2);
    (void) cursor.eraseSelection();
    cursor.activateSelection(false);
    REQUIRE(cursor.getText() == std::u16string(u"oree"));
    REQUIRE(cursor.getLineCount() == 1);

    CHECK(undoAll(cursor) == 1);
    CHECK(cursor.getText() == std::u16string(u"one\ntwo\nthree"));
    CHECK(cursor.getLineCount() == 3);
}

TEST_CASE("a backspace run inside one line is a single undo step") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"abcdef");

    cursor.moveToEndOfLine();
    for (auto i = 0; i < 3; ++i) {
        (void) cursor.eraseLeft();
    }
    REQUIRE(cursor.getText() == std::u16string(u"abc"));

    CHECK(undoAll(cursor) == 1);
    CHECK(cursor.getText() == std::u16string(u"abcdef"));
}

TEST_CASE("erasing a non-BMP character takes the whole pair and undo restores it") {
    const auto grin = std::u16string(u"\U0001F600");
    REQUIRE(grin.length() == 2);

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    const auto original = std::u16string(u"a") + grin + u"b";
    seed(cursor, original);

    cursor.moveToEndOfLine();
    (void) cursor.eraseLeft();                  // drops 'b'
    (void) cursor.eraseLeft();                  // must drop both code units of the pair
    REQUIRE(cursor.getText() == std::u16string(u"a"));
    REQUIRE_FALSE(hasLoneSurrogate(cursor.getText()));

    REQUIRE(undoAll(cursor) == 1);
    CHECK(cursor.getText() == original);
    CHECK_FALSE(hasLoneSurrogate(cursor.getText()));
}

TEST_CASE("undo across two pairs sharing a high surrogate does not split them") {
    // U+1F600 and U+1F601 differ only in their trailing code unit, so the common prefix of the two
    // buffer states ends *inside* the pair and the common suffix reaches back into it. Both ends
    // have to be widened off the pair, or the undo leaves an unencodable buffer behind.
    const auto grin = std::u16string(u"\U0001F600");
    const auto beam = std::u16string(u"\U0001F601");
    REQUIRE(grin[0] == beam[0]);
    REQUIRE(grin[1] != beam[1]);

    auto cursor = Cursor(std::make_unique<LineBuffer>());
    const auto original = std::u16string(u"a") + grin + u"b";
    seed(cursor, original);

    // Replace the emoji, the shape SearchCommand::replaceSelection produces
    select(cursor, 0, 1, 0, 3);
    (void) cursor.eraseSelection();
    cursor.activateSelection(false);
    (void) cursor.insert(beam);
    REQUIRE(cursor.getText() == std::u16string(u"a") + beam + u"b");
    REQUIRE_FALSE(hasLoneSurrogate(cursor.getText()));

    REQUIRE(undoAll(cursor) == 1);
    CHECK(cursor.getText() == original);
    CHECK_FALSE(hasLoneSurrogate(cursor.getText()));
}

TEST_CASE("the entry cap keeps the most recent steps and drops the oldest") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    auto max_undo = std::make_shared<CVarInt>(3);
    cursor.shareMaxHistoryDepth(max_undo);

    for (const auto *const text : {u"a", u"b", u"c", u"d", u"e", u"f"}) {
        appendAsNewGroup(cursor, text);
    }
    REQUIRE(cursor.getText() == std::u16string(u"abcdef"));

    // Six groups were made, three fit: undo reaches back exactly three steps and stops
    CHECK(undoAll(cursor) == 3);
    CHECK(cursor.getText() == std::u16string(u"abc"));
}

TEST_CASE("lowering the entry cap at runtime trims the history immediately") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    auto max_undo = std::make_shared<CVarInt>(10);
    cursor.shareMaxHistoryDepth(max_undo);

    for (const auto *const text : {u"a", u"b", u"c", u"d", u"e"}) {
        appendAsNewGroup(cursor, text);
    }

    // What the cvar command does: write the value, then let the change callback apply it
    max_undo->m_value = 2;
    cursor.setMaxHistoryDepth();

    CHECK(undoAll(cursor) == 2);
    CHECK(cursor.getText() == std::u16string(u"abc"));
}

TEST_CASE("loading content leaves nothing to undo") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());

    // Edit first, so there is a history that loading has to discard
    type(cursor, u"scratch");
    REQUIRE(undoAll(cursor) == 1);

    (void) cursor.loadContent(u"one\ntwo\nthree");
    CHECK(cursor.getText() == std::u16string(u"one\ntwo\nthree"));
    CHECK(cursor.getLineCount() == 3);
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    // The file is not an edit: undoing must not peel it back off, and redo must not reach the
    // buffer that was replaced
    CHECK_FALSE(undoStep(cursor));
    CHECK_FALSE(redoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"one\ntwo\nthree"));

    // Editing the loaded buffer still works, and undoes back to the loaded state
    cursor.moveToEndOfFile();
    type(cursor, u"!");
    REQUIRE(cursor.getText() == std::u16string(u"one\ntwo\nthree!"));
    REQUIRE(undoAll(cursor) == 1);
    CHECK(cursor.getText() == std::u16string(u"one\ntwo\nthree"));
}

TEST_CASE("undoing back to the saved state reports unmodified") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"hello");
    REQUIRE_FALSE(cursor.isModified());

    appendAsNewGroup(cursor, u" world");
    CHECK(cursor.isModified());

    // Back at the state the buffer was saved in, so it matches the disk again
    REQUIRE(undoStep(cursor));
    CHECK_FALSE(cursor.isModified());

    // And forward off it again
    REQUIRE(redoStep(cursor));
    CHECK(cursor.isModified());
}

TEST_CASE("the saved state can sit in the middle of the history") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    appendAsNewGroup(cursor, u"aaa");
    cursor.setModified(false);              // saved here, with history on both sides
    appendAsNewGroup(cursor, u"bbb");
    CHECK(cursor.isModified());

    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u"aaa"));
    CHECK_FALSE(cursor.isModified());

    // Undoing past the saved point is a difference from disk just as much as editing forward is
    REQUIRE(undoStep(cursor));
    CHECK(cursor.getText() == std::u16string(u""));
    CHECK(cursor.isModified());

    REQUIRE(redoStep(cursor));
    CHECK_FALSE(cursor.isModified());
}

TEST_CASE("an edit after undoing past the saved state strands it") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");

    appendAsNewGroup(cursor, u"aaa");
    cursor.setModified(false);
    appendAsNewGroup(cursor, u"bbb");
    REQUIRE(undoStep(cursor));
    REQUIRE(undoStep(cursor));
    REQUIRE(cursor.isModified());                  // below the saved state now

    // The new branch drops the redo stack, so the saved state is no longer reachable forward
    appendAsNewGroup(cursor, u"ccc");
    CHECK(cursor.isModified());
    CHECK(undoAll(cursor) == 1);
    CHECK(cursor.isModified());
}

TEST_CASE("a saved state trimmed out of the history stays modified") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"");
    REQUIRE_FALSE(cursor.isModified());

    auto max_undo = std::make_shared<CVarInt>(2);
    cursor.shareMaxHistoryDepth(max_undo);

    // Four groups through a two-deep history: the state the buffer was saved in falls off the end
    for (const auto *const text : {u"a", u"b", u"c", u"d"}) {
        appendAsNewGroup(cursor, text);
    }
    CHECK(cursor.isModified());

    // Undoing everything still reachable cannot get back to it, so it must not claim to be saved
    CHECK(undoAll(cursor) == 2);
    CHECK(cursor.getText() == std::u16string(u"ab"));
    CHECK(cursor.isModified());
}

TEST_CASE("loading content starts the buffer clean") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    type(cursor, u"scratch");
    REQUIRE(cursor.isModified());

    (void) cursor.loadContent(u"one\ntwo");
    CHECK_FALSE(cursor.isModified());

    type(cursor, u"x");
    CHECK(cursor.isModified());
    REQUIRE(undoAll(cursor) == 1);
    CHECK_FALSE(cursor.isModified());
}
