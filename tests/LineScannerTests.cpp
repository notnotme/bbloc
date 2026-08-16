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
#include <string>
#include <vector>

#include "TestSupport.h"

#include "core/base/LineScanner.h"


/**
 * @brief Finds the first occurrence of a term the obvious way, one candidate position at a time.
 *
 * Each character pair is folded at comparison time by a local fold written from the A-Z definition,
 * so "an empty term matches at from whenever it fits" falls out of the loop bounds rather than being
 * copied from the scanner.
 *
 * @param line The line to scan.
 * @param term The term to look for.
 * @param from The position to start scanning from.
 * @param caseSensitive Whether the comparison is case-sensitive.
 * @return The starting position, or std::u16string_view::npos when absent.
 */
static size_t referenceIndexOf(const std::u16string_view line, const std::u16string_view term, const size_t from, const bool caseSensitive) {
    const auto fold = [](const char16_t character) {
        return character >= u'A' && character <= u'Z' ? static_cast<char16_t>(character + (u'a' - u'A')) : character;
    };

    for (auto position = from; position + term.length() <= line.length(); ++position) {
        auto matched = true;
        for (auto offset = std::size_t{ 0 }; offset < term.length(); ++offset) {
            const auto left = caseSensitive ? line[position + offset] : fold(line[position + offset]);
            const auto right = caseSensitive ? term[offset] : fold(term[offset]);
            if (left != right) {
                matched = false;
                break;
            }
        }

        if (matched) {
            return position;
        }
    }

    return std::u16string_view::npos;
}

/**
 * @brief Finds the last occurrence strictly below a bound by running referenceIndexOf everywhere.
 *
 * @param line The line to scan.
 * @param term The term to look for.
 * @param limit The exclusive upper bound for the match start position.
 * @param caseSensitive Whether the comparison is case-sensitive.
 * @return The starting position, or std::u16string_view::npos when absent.
 */
static size_t referenceLastIndexOf(const std::u16string_view line, const std::u16string_view term, const size_t limit, const bool caseSensitive) {
    auto best = std::u16string_view::npos;
    for (auto position = std::size_t{ 0 }; position < limit && position + term.length() <= line.length(); ++position) {
        if (referenceIndexOf(line, term, position, caseSensitive) == position) {
            best = position;
        }
    }

    return best;
}

/**
 * @brief Decides self-overlap by trying to build a witness string carrying two overlapping occurrences.
 *
 * For each shift, the witness of length len+shift is filled cell by cell from an occurrence at 0 and
 * one at shift; the term overlaps itself exactly when some shift assigns no cell two different
 * characters. Deliberately not the shift-suffix comparison the scanner uses.
 *
 * @param term The term to test, already folded when the mode folds.
 * @return true when two occurrences of the term can overlap.
 */
static bool referenceSelfOverlap(const std::u16string_view term) {
    for (auto shift = std::size_t{ 1 }; shift < term.length(); ++shift) {
        auto witness = std::vector<char16_t>(term.length() + shift, u'\0');
        auto assigned = std::vector<bool>(term.length() + shift, false);

        auto conflict = false;
        for (const auto start : { std::size_t{ 0 }, shift }) {
            for (auto offset = std::size_t{ 0 }; offset < term.length(); ++offset) {
                if (assigned[start + offset] && witness[start + offset] != term[offset]) {
                    conflict = true;
                    break;
                }
                witness[start + offset] = term[offset];
                assigned[start + offset] = true;
            }
            if (conflict) {
                break;
            }
        }

        if (!conflict) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Renders an ASCII-only input so a failing sweep names the string it failed on.
 *
 * @param text The text to render.
 * @return The same text as bytes.
 */
static std::string ascii(const std::u16string_view text) {
    auto rendered = std::string{};
    for (const auto character : text) {
        rendered.push_back(static_cast<char>(character));
    }

    return rendered;
}

/**
 * @brief Visits every string up to a given length over an alphabet.
 *
 * Exhausting a small alphabet is what covers the sequences nobody thinks to write down — a term
 * overlapping itself only after the fold, a match ending exactly on the line's last unit, and so on.
 *
 * @param alphabet The characters to draw from.
 * @param maxLength The longest string to visit.
 * @param visit Called once per string, shortest first.
 * @return The number of strings visited.
 */
template <typename TVisitor>
static uint32_t forEachString(const std::u16string_view alphabet, const std::size_t maxLength, const TVisitor &visit) {
    auto text = std::u16string{};
    auto visited = uint32_t{ 0 };

    for (auto length = std::size_t{ 0 }; length <= maxLength; ++length) {
        auto combinations = std::size_t{ 1 };
        for (auto index = std::size_t{ 0 }; index < length; ++index) {
            combinations *= alphabet.length();
        }

        for (auto combination = std::size_t{ 0 }; combination < combinations; ++combination) {
            text.assign(length, u' ');
            auto remainder = combination;
            for (auto position = std::size_t{ 0 }; position < length; ++position) {
                text[position] = alphabet[remainder % alphabet.length()];
                remainder /= alphabet.length();
            }

            visit(std::u16string_view(text));
            ++visited;
        }
    }

    return visited;
}


TEST_CASE("an empty term matches wherever it fits and never overlaps itself") {
    for (const auto case_sensitive : { false, true }) {
        CAPTURE(case_sensitive);
        auto scanner = LineScanner(u"", case_sensitive);

        CHECK(scanner.termLength() == 0);
        CHECK(!scanner.isSelfOverlapping());

        scanner.setLine(u"abc");
        for (auto from = std::size_t{ 0 }; from <= 3; ++from) {
            CHECK(scanner.indexOf(from) == from);
        }
        CHECK(scanner.indexOf(4) == std::u16string_view::npos);
    }
}

TEST_CASE("self-overlap is exactly a proper prefix that is also a suffix") {
    SUBCASE("a single character cannot overlap itself") {
        CHECK(!LineScanner(u"a", true).isSelfOverlapping());
    }

    SUBCASE("a run of the same character overlaps itself") {
        CHECK(LineScanner(u"aa", true).isSelfOverlapping());
    }

    SUBCASE("a proper border makes the overlap") {
        // "aba" carries occurrences at 0 and 2 of "ababa": the border is the single "a"
        CHECK(LineScanner(u"aba", true).isSelfOverlapping());
    }

    SUBCASE("two distinct characters cannot overlap") {
        CHECK(!LineScanner(u"ab", true).isSelfOverlapping());
    }
}

TEST_CASE("isSelfOverlapping runs on the folded term") {
    // "Aa" and "aA" fold to "aa", so the overlap only exists when the scanner folds; a scanner
    // testing the raw term would answer the same in both modes
    CHECK(LineScanner(u"Aa", false).isSelfOverlapping());
    CHECK(LineScanner(u"aA", false).isSelfOverlapping());
    CHECK(!LineScanner(u"Aa", true).isSelfOverlapping());
    CHECK(!LineScanner(u"aA", true).isSelfOverlapping());
}

TEST_CASE("the fold is ASCII-only, with A and Z inside it and their neighbours outside") {
    SUBCASE("accented letters never fold") {
        auto scanner = LineScanner(u"É", false);
        scanner.setLine(u"é");
        CHECK(scanner.indexOf(0) == std::u16string_view::npos);

        CHECK(!LineScanner(u"Àà", false).isSelfOverlapping());
        CHECK(LineScanner(u"ÀÀ", false).isSelfOverlapping());
    }

    SUBCASE("fullwidth letters never fold") {
        auto scanner = LineScanner(u"Ａ", false);
        scanner.setLine(u"ａa");
        CHECK(scanner.indexOf(0) == std::u16string_view::npos);
    }

    SUBCASE("the range boundaries hold") {
        // '@' precedes 'A' and '[' follows 'Z'; folding either would shift it by the case distance
        auto at_scanner = LineScanner(u"@", false);
        at_scanner.setLine(u"`");
        CHECK(at_scanner.indexOf(0) == std::u16string_view::npos);

        auto bracket_scanner = LineScanner(u"[", false);
        bracket_scanner.setLine(u"{");
        CHECK(bracket_scanner.indexOf(0) == std::u16string_view::npos);

        auto first_scanner = LineScanner(u"A", false);
        first_scanner.setLine(u"a");
        CHECK(first_scanner.indexOf(0) == 0);

        auto last_scanner = LineScanner(u"Z", false);
        last_scanner.setLine(u"z");
        CHECK(last_scanner.indexOf(0) == 0);
    }
}

TEST_CASE("a surrogate-pair term is found by code-unit position and survives the fold") {
    for (const auto case_sensitive : { false, true }) {
        CAPTURE(case_sensitive);
        auto scanner = LineScanner(u"😀", case_sensitive);

        CHECK(scanner.termLength() == 2);

        scanner.setLine(u"ab😀cd");
        CHECK(scanner.indexOf(0) == 2);
        CHECK(scanner.indexOf(3) == std::u16string_view::npos);
    }
}

TEST_CASE("both sides fold under case-insensitive matching") {
    auto upper_term = LineScanner(u"ABC", false);
    upper_term.setLine(u"abc");
    CHECK(upper_term.indexOf(0) == 0);

    auto lower_term = LineScanner(u"abc", false);
    lower_term.setLine(u"ABC");
    CHECK(lower_term.indexOf(0) == 0);

    auto sensitive_upper = LineScanner(u"ABC", true);
    sensitive_upper.setLine(u"abc");
    CHECK(sensitive_upper.indexOf(0) == std::u16string_view::npos);

    auto sensitive_lower = LineScanner(u"abc", true);
    sensitive_lower.setLine(u"ABC");
    CHECK(sensitive_lower.indexOf(0) == std::u16string_view::npos);
}

TEST_CASE("lastIndexOf treats its limit as exclusive") {
    auto scanner = LineScanner(u"ab", true);
    scanner.setLine(u"abab");     // matches start at 0 and 2

    CHECK(scanner.lastIndexOf(0) == std::u16string_view::npos);

    // A match starting exactly at limit - 1 is found; at limit it is not
    CHECK(scanner.lastIndexOf(3) == 2);
    CHECK(scanner.lastIndexOf(2) == 0);

    // A limit far past the line still finds the final match
    CHECK(scanner.lastIndexOf(100) == 2);
}

TEST_CASE("indexOf starts exactly at its from offset") {
    auto scanner = LineScanner(u"ab", true);
    scanner.setLine(u"abab");

    // from on a match start finds it; one past misses it and finds the next
    CHECK(scanner.indexOf(0) == 0);
    CHECK(scanner.indexOf(1) == 2);
    CHECK(scanner.indexOf(3) == std::u16string_view::npos);

    // Overlapping occurrences are all visible when stepping by one
    auto overlapping = LineScanner(u"aa", true);
    overlapping.setLine(u"aaaa");
    CHECK(overlapping.indexOf(0) == 0);
    CHECK(overlapping.indexOf(1) == 1);
    CHECK(overlapping.indexOf(2) == 2);
    CHECK(overlapping.indexOf(3) == std::u16string_view::npos);
}

TEST_CASE("one scanner scans many lines in turn") {
    // searchForward walks a stateful scanner down the buffer, so a stale folded scratch would leak
    // one line's matches into the next
    auto scanner = LineScanner(u"ab", false);

    scanner.setLine(u"ZZab");
    CHECK(scanner.indexOf(0) == 2);

    scanner.setLine(u"xxxx");
    CHECK(scanner.indexOf(0) == std::u16string_view::npos);

    scanner.setLine(u"ABxx");
    CHECK(scanner.indexOf(0) == 0);

    scanner.setLine(u"");
    CHECK(scanner.indexOf(0) == std::u16string_view::npos);

    scanner.setLine(u"xxAB");
    CHECK(scanner.lastIndexOf(100) == 2);
}

TEST_CASE("termLength reports the constructor argument's length in code units") {
    for (const auto case_sensitive : { false, true }) {
        CAPTURE(case_sensitive);
        CHECK(LineScanner(u"", case_sensitive).termLength() == 0);
        CHECK(LineScanner(u"AbC", case_sensitive).termLength() == 3);
        CHECK(LineScanner(u"a😀b", case_sensitive).termLength() == 4);
    }
}

TEST_CASE("matching agrees with a per-position folding scan") {
    // Every term of up to four characters against every line of up to six, over an alphabet where
    // the fold both matters ('A' meets 'a') and must not fire ('b' stays 'b'), in both modes, with
    // every from and limit up to one past the interesting range
    auto mismatches = 0;
    auto lookups = uint32_t{ 0 };

    const auto terms = forEachString(u"abA", 4, [&mismatches, &lookups](const std::u16string_view term) {
        forEachString(u"abA", 6, [&mismatches, &lookups, term](const std::u16string_view line) {
            for (const auto case_sensitive : { false, true }) {
                auto scanner = LineScanner(term, case_sensitive);
                scanner.setLine(line);

                for (auto from = std::size_t{ 0 }; from <= line.length() + 1; ++from) {
                    const auto actual = scanner.indexOf(from);
                    const auto expected = referenceIndexOf(line, term, from, case_sensitive);
                    if (actual != expected) {
                        ++mismatches;
                        if (mismatches <= 5) {
                            const auto rendered_term = ascii(term);
                            const auto rendered_line = ascii(line);
                            CAPTURE(rendered_term);
                            CAPTURE(rendered_line);
                            CAPTURE(case_sensitive);
                            CAPTURE(from);
                            CHECK(actual == expected);
                        }
                    }
                    ++lookups;
                }

                for (auto limit = std::size_t{ 0 }; limit <= line.length() + 2; ++limit) {
                    const auto actual = scanner.lastIndexOf(limit);
                    const auto expected = referenceLastIndexOf(line, term, limit, case_sensitive);
                    if (actual != expected) {
                        ++mismatches;
                        if (mismatches <= 5) {
                            const auto rendered_term = ascii(term);
                            const auto rendered_line = ascii(line);
                            CAPTURE(rendered_term);
                            CAPTURE(rendered_line);
                            CAPTURE(case_sensitive);
                            CAPTURE(limit);
                            CHECK(actual == expected);
                        }
                    }
                    ++lookups;
                }
            }
        });
    });

    CHECK(terms == 121);
    CHECK(lookups == 4233790);
    CHECK(mismatches == 0);
}

TEST_CASE("self-overlap agrees with building a witness of two overlapping occurrences") {
    const auto fold = [](const char16_t character) {
        return character >= u'A' && character <= u'Z' ? static_cast<char16_t>(character + (u'a' - u'A')) : character;
    };

    const auto check_term = [&fold](const std::u16string_view term) {
        for (const auto case_sensitive : { false, true }) {
            // The scanner folds before testing, so the insensitive reference runs on the folded term
            auto folded = std::u16string(term);
            if (!case_sensitive) {
                for (auto &character : folded) {
                    character = fold(character);
                }
            }

            const auto actual = LineScanner(term, case_sensitive).isSelfOverlapping();
            const auto expected = referenceSelfOverlap(folded);
            if (actual != expected) {
                const auto rendered_term = ascii(term);
                CAPTURE(rendered_term);
                CAPTURE(case_sensitive);
                CHECK(actual == expected);
            }
        }
    };

    CHECK(forEachString(u"ab", 6, check_term) == 127);
    CHECK(forEachString(u"abA", 4, check_term) == 121);
}
