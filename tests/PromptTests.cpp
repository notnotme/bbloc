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

#include "core/cursor/PromptCursor.h"


/** The emoji every surrogate case steps over: one character, two code units. */
static constexpr auto EMOJI = std::u16string_view(u"\U0001F600");

/**
 * @brief Returns the prompt text as a string doctest can print on a failure.
 *
 * The class hands out a view, and the StringMaker in TestSupport.h is registered for the owning
 * string, so a failing CHECK on a raw view would report "{?}" instead of the text.
 *
 * @param cursor The cursor to read.
 * @return A copy of the prompt content.
 */
static std::u16string text(const PromptCursor &cursor) {
    return std::u16string(cursor.getString());
}

/**
 * @brief Fills a fresh prompt with text, leaving the caret at the end as typing would.
 *
 * @param cursor The cursor to seed; must be empty.
 * @param characters The text to insert.
 */
static void seedPrompt(PromptCursor &cursor, const std::u16string_view characters) {
    cursor.insert(characters);
}


TEST_CASE("a fresh prompt is empty with the caret at the origin") {
    auto cursor = PromptCursor();

    CHECK(text(cursor) == std::u16string(u""));
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("inserting at the caret carries the caret along") {
    auto cursor = PromptCursor();

    // The prompt is fed one keystroke at a time by Prompt::onTextInput, so each insert has to leave
    // the caret ready for the next one
    cursor.insert(u"o");
    cursor.insert(u"p");
    cursor.insert(u"en");

    CHECK(text(cursor) == std::u16string(u"open"));
    CHECK(cursor.getColumn() == 4);
}

TEST_CASE("inserting in the middle splits the text at the caret") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"oen");

    cursor.setPosition(1);
    cursor.insert(u"p");

    CHECK(text(cursor) == std::u16string(u"open"));

    // The caret follows the inserted run, not the text that was pushed to its right
    CHECK(cursor.getColumn() == 2);
}

TEST_CASE("an insert of several characters moves the caret by all of them") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"ab");

    // Completion pastes a whole word at once, which must not land the caret mid-word
    cursor.setPosition(1);
    cursor.insert(u"XYZ");

    CHECK(text(cursor) == std::u16string(u"aXYZb"));
    CHECK(cursor.getColumn() == 4);
}

TEST_CASE("eraseLeft takes the character before the caret") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"open");

    cursor.eraseLeft();

    CHECK(text(cursor) == std::u16string(u"ope"));
    CHECK(cursor.getColumn() == 3);
}

TEST_CASE("eraseRight takes the character at the caret and leaves it put") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"open");

    cursor.setPosition(1);
    cursor.eraseRight();

    CHECK(text(cursor) == std::u16string(u"oen"));

    // Delete removes what is ahead, so the caret has nothing to move over
    CHECK(cursor.getColumn() == 1);
}

TEST_CASE("eraseLeft takes a whole surrogate pair") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, std::u16string(u"a").append(EMOJI));

    cursor.eraseLeft();

    // Both units go at once: half a pair is text that can no longer be encoded back to UTF-8
    CHECK(text(cursor) == std::u16string(u"a"));
    CHECK(cursor.getColumn() == 1);
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));
}

TEST_CASE("eraseRight takes a whole surrogate pair") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, std::u16string(u"a").append(EMOJI).append(u"b"));

    cursor.setPosition(1);
    cursor.eraseRight();

    CHECK(text(cursor) == std::u16string(u"ab"));
    CHECK(cursor.getColumn() == 1);
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));
}

TEST_CASE("erasing a pair that ends the text leaves nothing behind") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, EMOJI);

    // charLengthAfter needs the unit after the caret to exist, so a pair with no tail is its own case
    cursor.moveToStart();
    cursor.eraseRight();

    CHECK(text(cursor) == std::u16string(u""));
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));

    seedPrompt(cursor, EMOJI);
    cursor.moveToEnd();
    cursor.eraseLeft();

    CHECK(text(cursor) == std::u16string(u""));
    CHECK(cursor.getColumn() == 0);
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));
}

TEST_CASE("moves step a surrogate pair as one character") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, std::u16string(u"a").append(EMOJI).append(u"b"));
    cursor.moveToStart();

    // The emoji occupies columns 1 and 2; the caret must never come to rest between them
    cursor.moveRight();
    CHECK(cursor.getColumn() == 1);
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
}

TEST_CASE("moving past either end is a no-op") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"ab");

    // The column is unsigned, so a left move at the origin would wrap to a huge value rather than
    // fail visibly
    cursor.moveToStart();
    cursor.moveLeft();
    cursor.moveLeft();
    CHECK(cursor.getColumn() == 0);

    cursor.moveToEnd();
    cursor.moveRight();
    cursor.moveRight();
    CHECK(cursor.getColumn() == 2);

    CHECK(text(cursor) == std::u16string(u"ab"));
}

TEST_CASE("moveToStart and moveToEnd jump to the ends") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, std::u16string(u"ab").append(EMOJI));

    cursor.moveToStart();
    CHECK(cursor.getColumn() == 0);

    // The end is counted in code units, so the trailing pair counts for two
    cursor.moveToEnd();
    CHECK(cursor.getColumn() == 4);
}

TEST_CASE("setPosition accepts every column up to the end and throws past it") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"open");

    cursor.setPosition(0);
    CHECK(cursor.getColumn() == 0);

    // The column just after the last character is a legal caret spot, unlike the one after that
    cursor.setPosition(4);
    CHECK(cursor.getColumn() == 4);

    CHECK_THROWS_AS(cursor.setPosition(5), std::runtime_error);

    // A refused position leaves the caret where it was
    CHECK(cursor.getColumn() == 4);
}

TEST_CASE("setPosition snaps a column landing inside a surrogate pair") {
    // The only way this cursor can be handed a column it did not compute itself: its own moves
    // and erases step whole characters through charLengthBefore/charLengthAfter.
    auto cursor = PromptCursor();
    cursor.insert(std::u16string(u"a").append(EMOJI).append(u"b"));

    cursor.setPosition(2);      // between the two units of the pair
    CHECK(cursor.getColumn() == 1);

    // The columns on either side of the pair are untouched
    cursor.setPosition(1);
    CHECK(cursor.getColumn() == 1);
    cursor.setPosition(3);
    CHECK(cursor.getColumn() == 3);
}

TEST_CASE("a caret placed inside a surrogate pair cannot strand half of one") {
    // The failure this guards: erasing from a mid-pair caret used to take a single code unit,
    // leaving a lone surrogate behind and a prompt that no longer encodes to UTF-8.
    // The snap lands the caret on the pair's *lead* unit, so the erases then act on whole
    // characters either side of it — "a" to the left, the emoji to the right.
    auto cursor = PromptCursor();
    cursor.insert(std::u16string(u"a").append(EMOJI).append(u"b"));

    cursor.setPosition(2);
    cursor.eraseLeft();
    CHECK(text(cursor) == std::u16string(u"a").append(EMOJI).append(u"b").erase(0, 1));
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));

    cursor.clear();
    cursor.insert(std::u16string(u"a").append(EMOJI).append(u"b"));
    cursor.setPosition(2);
    cursor.eraseRight();
    CHECK(text(cursor) == std::u16string(u"ab"));
    CHECK_FALSE(hasLoneSurrogate(cursor.getString()));
}

TEST_CASE("erasing at either end is a no-op") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"ab");

    cursor.moveToStart();
    cursor.eraseLeft();
    CHECK(text(cursor) == std::u16string(u"ab"));
    CHECK(cursor.getColumn() == 0);

    cursor.moveToEnd();
    cursor.eraseRight();
    CHECK(text(cursor) == std::u16string(u"ab"));
    CHECK(cursor.getColumn() == 2);
}

TEST_CASE("erasing an empty prompt is a no-op") {
    auto cursor = PromptCursor();

    cursor.eraseLeft();
    cursor.eraseRight();

    CHECK(text(cursor) == std::u16string(u""));
    CHECK(cursor.getColumn() == 0);
}

TEST_CASE("clear empties the text and returns the caret to the origin") {
    auto cursor = PromptCursor();
    seedPrompt(cursor, u"open file");

    cursor.clear();

    // Every command that reuses the prompt goes through clear(), so a caret left behind would point
    // past the end of the next input
    CHECK(text(cursor) == std::u16string(u""));
    CHECK(cursor.getColumn() == 0);

    cursor.insert(u"x");
    CHECK(text(cursor) == std::u16string(u"x"));
    CHECK(cursor.getColumn() == 1);
}
