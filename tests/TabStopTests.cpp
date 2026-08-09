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
#include <array>

#include "TestSupport.h"

#include "core/theme/TabStop.h"


/** The tab widths every case sweeps: the degenerate one, the two common ones, and a wide one. */
static constexpr auto TAB_WIDTHS = std::array<uint32_t, 4>{ 1, 2, 4, 8 };

/** Line prefixes mixing tabs and text the way indented code does. */
static constexpr auto SAMPLES = std::array<std::u16string_view, 12>{
    u"",
    u"a",
    u"abcdefgh",
    u"\t",
    u"\t\t\t",
    u"\ta",
    u"a\t",
    u"abc\t",
    u"abcd\t",
    u"abcde\t",
    u"\tif (x) {\t// comment",
    u"a\tbb\tccc\t\tdddd\t",
};

/**
 * @brief Measures a prefix the slow, obvious way, one character at a time.
 *
 * The function under test folds this walk into runs delimited by std::find, so the walk is the
 * specification it has to keep agreeing with. It derives the stop by stepping to the next multiple
 * rather than by the arithmetic in nextTabStop, so the two cannot be wrong together.
 *
 * @param text The line prefix to measure; it must start at visual column 0.
 * @param tabWidth The tab width in columns, at least 1.
 * @return The visual width of the prefix, in columns.
 */
static uint32_t referenceVisualColumns(const std::u16string_view text, const uint32_t tabWidth) {
    auto visual_column = uint32_t{ 0 };
    for (const auto character : text) {
        if (character == u'\t') {
            // A tab is at least one column wide and ends on a multiple of the width
            do {
                ++visual_column;
            } while (visual_column % tabWidth != 0);
        } else {
            ++visual_column;
        }
    }

    return visual_column;
}


TEST_CASE("a tab stop is the next multiple of the tab width") {
    CHECK(nextTabStop(0, 4) == 4);
    CHECK(nextTabStop(1, 4) == 4);
    CHECK(nextTabStop(3, 4) == 4);

    // A tab drawn on a stop is a full width wide, never zero: the caret has to move
    CHECK(nextTabStop(4, 4) == 8);
    CHECK(nextTabStop(8, 4) == 12);

    CHECK(nextTabStop(9, 8) == 16);
    CHECK(nextTabStop(16, 8) == 24);
}

TEST_CASE("a tab width of one makes every tab a single column") {
    for (auto column = uint32_t{ 0 }; column < 10; ++column) {
        CHECK(nextTabStop(column, 1) == column + 1);
    }
}

TEST_CASE("a prefix with no tab measures as its own length") {
    // This is the fast path the callers take on a line whose tab count is zero, so the two ways of
    // measuring must give the same answer or the glyphs and the caret drift apart
    for (const auto tab_width : TAB_WIDTHS) {
        CHECK(visualColumns(u"", tab_width) == 0);
        CHECK(visualColumns(u"a", tab_width) == 1);
        CHECK(visualColumns(u"abcdefgh", tab_width) == 8);
    }
}

TEST_CASE("a tab pushes the prefix to its stop") {
    // Widths taken from the tab_width cvar's usual values, and text runs on either side of the tab
    CHECK(visualColumns(u"\t", 4) == 4);
    CHECK(visualColumns(u"a\t", 4) == 4);
    CHECK(visualColumns(u"abc\t", 4) == 4);

    // A tab landing on a stop still advances a whole width
    CHECK(visualColumns(u"abcd\t", 4) == 8);
    CHECK(visualColumns(u"abcde\t", 4) == 8);

    // Text after the tab counts from where the tab left off
    CHECK(visualColumns(u"a\tb", 4) == 5);
    CHECK(visualColumns(u"\t\t", 4) == 8);
    CHECK(visualColumns(u"\ta\t", 4) == 8);
}

TEST_CASE("measuring a prefix agrees with walking it character by character") {
    for (const auto sample : SAMPLES) {
        for (const auto tab_width : TAB_WIDTHS) {
            CAPTURE(tab_width);
            CHECK(visualColumns(sample, tab_width) == referenceVisualColumns(sample, tab_width));
        }
    }
}

TEST_CASE("every prefix of a line is wider than the one before it") {
    // The glyph walk and columnAtPixel step through a line prefix by prefix, so a character that
    // took no width would give two columns the same pixel and make the caret ambiguous
    for (const auto sample : SAMPLES) {
        for (const auto tab_width : TAB_WIDTHS) {
            CAPTURE(tab_width);
            for (auto length = std::size_t{ 1 }; length <= sample.length(); ++length) {
                CHECK(visualColumns(sample.substr(0, length), tab_width)
                    > visualColumns(sample.substr(0, length - 1), tab_width));
            }
        }
    }
}
