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

}


TEST_CASE("a seeded buffer holds its text and an empty history") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    seed(cursor, u"hello");

    CHECK(cursor.getText() == std::u16string(u"hello"));
    CHECK(cursor.getLineCount() == 1);
    CHECK(cursor.getLine() == 0);
    CHECK(cursor.getColumn() == 0);

    // Nothing to undo: the seeding edit was wiped with the history
    CHECK_FALSE(cursor.undo().has_value());
}
