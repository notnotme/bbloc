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
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

#include "TestSupport.h"

#include "core/base/LineEnding.h"


/**
 * @brief Detects the convention the obvious way, straight from the raw bytes.
 *
 * Counts the "\r\n" occurrences against the lone "\n" occurrences and lets the larger side win.
 * That is the majority rule restated without any notion of lines, which is what makes it an
 * independent derivation from the getline-shaped counting readFile does.
 *
 * @param raw The file bytes.
 * @return The convention a strict majority of the newlines carry.
 */
static LineEnding referenceDetect(const std::string_view raw) {
    auto crlf = uint32_t{ 0 };
    auto lone = uint32_t{ 0 };
    for (auto index = std::size_t{ 0 }; index < raw.length(); ++index) {
        if (raw[index] != '\n') {
            continue;
        }
        if (index > 0 && raw[index - 1] == '\r') {
            ++crlf;
        } else {
            ++lone;
        }
    }

    return crlf > lone ? LineEnding::Crlf : LineEnding::Lf;
}

/**
 * @brief Runs OpenFileCommand::readFile's exact getline/pop/count loop over in-memory bytes.
 *
 * The loop is reproduced statement for statement — capture the trailing '\r' before popping it,
 * then let only a line the stream did not end on vote — because that decision is precisely what
 * this file tests: a stray final "\r" at EOF must not count as a CRLF ending.
 *
 * @param raw The file bytes.
 * @return What readFile would report for a file holding those bytes.
 */
static LineEnding simulateReadFile(const std::string &raw) {
    auto ifs = std::istringstream(raw);
    auto line = std::string{};
    auto crlf_line_count = uint32_t{ 0 };
    auto newline_line_count = uint32_t{ 0 };

    while (getline(ifs, line)) {
        const auto had_cr = !line.empty() && line.back() == '\r';
        if (had_cr) {
            line.pop_back();
        }

        if (!ifs.eof() && !ifs.fail()) {
            ++newline_line_count;
            if (had_cr) {
                ++crlf_line_count;
            }
        }
    }

    return detectLineEnding(crlf_line_count, newline_line_count);
}

/**
 * @brief Renders raw bytes so a failing sweep names the string it failed on.
 *
 * @param raw The bytes to render.
 * @return The same bytes with "\n" and "\r" spelled out.
 */
static std::string escaped(const std::string_view raw) {
    auto rendered = std::string{};
    for (const auto character : raw) {
        if (character == '\n') {
            rendered.append("\\n");
        } else if (character == '\r') {
            rendered.append("\\r");
        } else {
            rendered.push_back(character);
        }
    }

    return rendered;
}

/**
 * @brief Visits every byte string up to a given length over an alphabet.
 *
 * Exhausting a small alphabet is what covers the sequences nobody thinks to write down — a "\r"
 * with no "\n" behind it, a file of nothing but separators, a "\r" sitting right at EOF.
 *
 * @param alphabet The bytes to draw from.
 * @param maxLength The longest string to visit.
 * @param visit Called once per string, shortest first.
 * @return The number of strings visited.
 */
template <typename TVisitor>
static uint32_t forEachString(const std::string_view alphabet, const std::size_t maxLength, const TVisitor &visit) {
    auto text = std::string{};
    auto visited = uint32_t{ 0 };

    for (auto length = std::size_t{ 0 }; length <= maxLength; ++length) {
        auto combinations = std::size_t{ 1 };
        for (auto index = std::size_t{ 0 }; index < length; ++index) {
            combinations *= alphabet.length();
        }

        for (auto combination = std::size_t{ 0 }; combination < combinations; ++combination) {
            text.assign(length, ' ');
            auto remainder = combination;
            for (auto position = std::size_t{ 0 }; position < length; ++position) {
                text[position] = alphabet[remainder % alphabet.length()];
                remainder /= alphabet.length();
            }

            visit(std::string_view(text));
            ++visited;
        }
    }

    return visited;
}


TEST_CASE("the detection agrees with a raw-byte majority count on every small file") {
    auto mismatches = 0;
    const auto visited = forEachString("a\n\r", 7, [&mismatches](const std::string_view raw) {
        if (simulateReadFile(std::string(raw)) != referenceDetect(raw)) {
            ++mismatches;
            if (mismatches <= 5) {
                const auto rendered = escaped(raw);
                CAPTURE(rendered);
                CHECK(simulateReadFile(std::string(raw)) == referenceDetect(raw));
            }
        }
    });

    CHECK(visited == 3280);
    CHECK(mismatches == 0);
}

TEST_CASE("applying CRLF reproduces a purely-CRLF file the read loop stripped") {
    // Every LF-separated text stands for the CRLF file it was stripped from; rewriting it back
    // must give the original bytes, or a saved CRLF file would not round-trip through the editor
    forEachString("a\n", 6, [](const std::string_view stripped) {
        auto original = std::string{};
        for (const auto character : stripped) {
            if (character == '\n') {
                original.append("\r\n");
            } else {
                original.push_back(character);
            }
        }

        const auto rendered = escaped(stripped);
        CAPTURE(rendered);
        CHECK(applyLineEnding(std::string(stripped), LineEnding::Crlf) == original);
    });
}

TEST_CASE("applying LF changes nothing") {
    forEachString("a\n", 6, [](const std::string_view stripped) {
        const auto rendered = escaped(stripped);
        CAPTURE(rendered);
        CHECK(applyLineEnding(std::string(stripped), LineEnding::Lf) == stripped);
    });
}

TEST_CASE("a CRLF result never holds a line feed without its carriage return") {
    forEachString("a\n", 6, [](const std::string_view stripped) {
        const auto rewritten = applyLineEnding(std::string(stripped), LineEnding::Crlf);
        for (auto index = std::size_t{ 0 }; index < rewritten.length(); ++index) {
            if (rewritten[index] == '\n') {
                const auto rendered = escaped(stripped);
                CAPTURE(rendered);
                REQUIRE(index > 0);
                CHECK(rewritten[index - 1] == '\r');
            }
        }
    });
}

TEST_CASE("an empty file and a single unterminated line read as LF") {
    CHECK(simulateReadFile("") == LineEnding::Lf);
    CHECK(simulateReadFile("a") == LineEnding::Lf);
    CHECK(detectLineEnding(0, 0) == LineEnding::Lf);
}

TEST_CASE("one CRLF line is already a majority") {
    CHECK(simulateReadFile("a\r\n") == LineEnding::Crlf);
    CHECK(detectLineEnding(1, 1) == LineEnding::Crlf);
}

TEST_CASE("two CRLF lines outvote one LF line") {
    CHECK(simulateReadFile("a\r\nb\r\nc\n") == LineEnding::Crlf);
    CHECK(detectLineEnding(2, 3) == LineEnding::Crlf);
}

TEST_CASE("a tie is not a majority, so it stays LF") {
    // The majority is strict on purpose: rewriting a file's endings needs a reason, and a file
    // that cannot make its mind up gives none
    CHECK(simulateReadFile("a\r\nb\n") == LineEnding::Lf);
    CHECK(detectLineEnding(1, 2) == LineEnding::Lf);
}

TEST_CASE("a buffer defaults to LF and keeps a set convention across a reload") {
    auto cursor = Cursor(std::make_unique<LineBuffer>());
    CHECK(cursor.getLineEnding() == LineEnding::Lf);

    cursor.setLineEnding(LineEnding::Crlf);
    CHECK(cursor.getLineEnding() == LineEnding::Crlf);

    // loadContent installs the text only; the convention is the caller's to set, in either order
    (void) cursor.loadContent(u"one\ntwo");
    CHECK(cursor.getLineEnding() == LineEnding::Crlf);
}
