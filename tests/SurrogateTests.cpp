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
#include <vector>

#include "TestSupport.h"

#include "core/cursor/SurrogatePair.h"


/** The emoji every case steps over: one character, two code units. */
static constexpr auto EMOJI = std::u16string_view(u"\U0001F600");

/** The halves of that emoji, used on their own to build the damaged lines. */
static constexpr auto LEAD = char16_t{ 0xD83D };
static constexpr auto TRAIL = char16_t{ 0xDE00 };

/**
 * @brief Walks a line forward, one character at a time, and reports the columns it stopped at.
 *
 * This is how Cursor::moveRight crosses a line, so the columns it visits are the only ones an edit
 * can start from.
 *
 * @param text The line to walk.
 * @return The visited columns, from 0 to the length of the line.
 */
static std::vector<uint32_t> walkForward(const std::u16string_view text) {
    auto columns = std::vector<uint32_t>{ 0 };
    auto column = uint32_t{ 0 };
    while (column < text.length()) {
        column += charLengthAfter(text, column);
        columns.push_back(column);
    }

    return columns;
}

/**
 * @brief Walks a line backward the same way, and reports the columns in increasing order.
 *
 * @param text The line to walk.
 * @return The visited columns, from 0 to the length of the line.
 */
static std::vector<uint32_t> walkBackward(const std::u16string_view text) {
    auto columns = std::vector<uint32_t>{ static_cast<uint32_t>(text.length()) };
    auto column = static_cast<uint32_t>(text.length());
    while (column > 0) {
        column -= charLengthBefore(text, column);
        columns.push_back(column);
    }

    std::ranges::reverse(columns);
    return columns;
}


TEST_CASE("a character out of the basic plane measures two code units") {
    const auto line = std::u16string(u"a").append(EMOJI).append(u"b");

    // Ahead of the caret: the lead unit answers for the whole pair, the letters for themselves
    CHECK(charLengthAfter(line, 0) == 1);
    CHECK(charLengthAfter(line, 1) == 2);
    CHECK(charLengthAfter(line, 3) == 1);

    // Behind the caret: the column after the trail unit is the one that sees the pair
    CHECK(charLengthBefore(line, 1) == 1);
    CHECK(charLengthBefore(line, 3) == 2);
    CHECK(charLengthBefore(line, 4) == 1);
}

TEST_CASE("measuring at the ends of a line reads nothing past it") {
    const auto line = std::u16string(u"a").append(EMOJI);

    // Nothing precedes column 0 and nothing follows the length, and both have to answer without
    // indexing outside the view
    CHECK(charLengthBefore(line, 0) == 1);
    CHECK(charLengthAfter(line, static_cast<uint32_t>(line.length())) == 1);

    // The trail unit of a pair sitting at the end of the line has no partner ahead of it
    CHECK(charLengthAfter(line, 2) == 1);

    CHECK(charLengthBefore(u"", 0) == 1);
    CHECK(charLengthAfter(u"", 0) == 1);
}

TEST_CASE("a surrogate with no partner measures as one code unit") {
    // Text this damaged cannot be written to disk, but it can be reached while an edit is halfway
    // through, and the measures must not pair a half with whatever sits next to it
    const auto lone_lead = std::u16string(1, LEAD).append(u"b");
    CHECK(charLengthAfter(lone_lead, 0) == 1);
    CHECK(charLengthBefore(lone_lead, 1) == 1);

    const auto lone_trail = std::u16string(u"a").append(1, TRAIL);
    CHECK(charLengthAfter(lone_trail, 1) == 1);
    CHECK(charLengthBefore(lone_trail, 2) == 1);

    // Two leads in a row are two damaged characters, not one pair
    const auto two_leads = std::u16string(2, LEAD);
    CHECK(charLengthAfter(two_leads, 0) == 1);
    CHECK(charLengthBefore(two_leads, 2) == 1);
}

TEST_CASE("a column inside a pair is pulled back to the character it splits") {
    const auto line = std::u16string(u"a").append(EMOJI).append(u"b");

    // Column 2 sits between the two halves: editing from there would split the emoji
    CHECK(snapToCharBoundary(line, 2) == 1);

    // Every other column of the line is already a boundary
    CHECK(snapToCharBoundary(line, 0) == 0);
    CHECK(snapToCharBoundary(line, 1) == 1);
    CHECK(snapToCharBoundary(line, 3) == 3);
    CHECK(snapToCharBoundary(line, 4) == 4);
}

TEST_CASE("snapping a column leaves nothing left to snap") {
    const auto line = std::u16string(u"ab").append(EMOJI).append(u"c").append(EMOJI).append(EMOJI);

    // A vertical move clamps a column onto a shorter line and hands the result straight to an edit,
    // so one pass has to be enough
    for (auto column = uint32_t{ 0 }; column <= line.length(); ++column) {
        CAPTURE(column);
        const auto snapped = snapToCharBoundary(line, column);
        CHECK(snapToCharBoundary(line, snapped) == snapped);
        CHECK(snapped <= column);
        CHECK(column - snapped <= 1);
    }
}

TEST_CASE("snapping reads nothing outside the line") {
    const auto line = std::u16string(u"a").append(EMOJI);

    // The length is a legal caret column and the line ends on a trail unit: the bound is what keeps
    // the view from being indexed at its own size
    CHECK(snapToCharBoundary(line, static_cast<uint32_t>(line.length())) == line.length());
    CHECK(snapToCharBoundary(u"", 0) == 0);

    // A trail unit at column 0 has nothing before it to pair with
    const auto lone_trail = std::u16string(1, TRAIL).append(u"b");
    CHECK(snapToCharBoundary(lone_trail, 0) == 0);
    CHECK(snapToCharBoundary(lone_trail, 1) == 1);

    // Nor does one following an ordinary character: it is damaged text, not a pair, and the caret
    // sits on a boundary already
    const auto orphan_trail = std::u16string(u"a").append(1, TRAIL).append(u"b");
    CHECK(snapToCharBoundary(orphan_trail, 1) == 1);
    CHECK(snapToCharBoundary(orphan_trail, 2) == 2);
}

TEST_CASE("both walks cross a line through the same boundaries") {
    const auto line = std::u16string(u"ab").append(EMOJI).append(u"c").append(EMOJI).append(EMOJI).append(u"d");

    const auto forward = walkForward(line);
    const auto backward = walkBackward(line);

    // Moving right then left again must retrace the caret's steps, so the two measures have to
    // agree on where the characters are
    CHECK(forward == backward);

    // The walk ends on the line, not past it, and never stops inside a character
    REQUIRE(!forward.empty());
    CHECK(forward.back() == line.length());
    for (const auto column : forward) {
        CAPTURE(column);
        CHECK(snapToCharBoundary(line, column) == column);
    }
}
