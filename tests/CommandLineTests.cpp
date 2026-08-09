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

#include "core/base/CommandLine.h"


/**
 * @brief Tokenizes and copies the result, so a case can compare it without worrying about lifetimes.
 *
 * @param input The command line to tokenize.
 * @return The tokens, as owning strings.
 */
static std::vector<std::u16string> tokensOf(const std::u16string_view input) {
    auto tokens = std::vector<std::u16string_view>{};
    CommandLine::tokenize(input, tokens);

    return std::vector<std::u16string>(tokens.begin(), tokens.end());
}

/**
 * @brief Splits and copies the result, for the same reason.
 *
 * @param input The text to split.
 * @param delimiter The delimiter to split on.
 * @return The parts, as owning strings.
 */
static std::vector<std::u16string> partsOf(const std::u16string_view input, const char16_t delimiter) {
    const auto parts = CommandLine::split(input, delimiter);

    return std::vector<std::u16string>(parts.begin(), parts.end());
}

/**
 * @brief Tokenizes a command line the obvious way, one character at a time.
 *
 * The rules, stated rather than scanned: spaces separate tokens and runs of them produce no empty
 * token; a token that *starts* with a quote runs to the next quote, which is consumed, and to the
 * end of the line if there is none; a quote anywhere else is an ordinary character; inside a quoted
 * token a space is an ordinary character; and a closing quote ends the token, so text touching it
 * on the right starts a new one.
 *
 * @param input The command line to tokenize.
 * @return The tokens, as owning strings.
 */
static std::vector<std::u16string> referenceTokenize(const std::u16string_view input) {
    enum class State { Between, Word, Quoted };

    auto tokens = std::vector<std::u16string>{};
    auto current = std::u16string{};
    auto state = State::Between;

    for (const auto character : input) {
        switch (state) {
            case State::Between:
                if (character == u' ') {
                    break;
                }
                current.clear();
                if (character == u'"') {
                    state = State::Quoted;
                } else {
                    state = State::Word;
                    current.push_back(character);
                }
                break;

            case State::Word:
                if (character == u' ') {
                    tokens.push_back(current);
                    state = State::Between;
                } else {
                    current.push_back(character);
                }
                break;

            case State::Quoted:
                if (character == u'"') {
                    tokens.push_back(current);
                    state = State::Between;
                } else {
                    current.push_back(character);
                }
                break;
        }
    }

    if (state != State::Between) {
        // A word running to the end of the line, or a quote nobody closed
        tokens.push_back(current);
    }

    return tokens;
}

/**
 * @brief Splits text the obvious way, dropping the empty parts a run of delimiters would produce.
 *
 * @param input The text to split.
 * @param delimiter The delimiter to split on.
 * @return The parts, as owning strings.
 */
static std::vector<std::u16string> referenceSplit(const std::u16string_view input, const char16_t delimiter) {
    auto parts = std::vector<std::u16string>{};
    auto current = std::u16string{};

    for (const auto character : input) {
        if (character != delimiter) {
            current.push_back(character);
            continue;
        }

        if (!current.empty()) {
            parts.push_back(current);
            current.clear();
        }
    }

    if (!current.empty()) {
        parts.push_back(current);
    }

    return parts;
}

/**
 * @brief Renders an ASCII-only input so a failing sweep names the string it failed on.
 *
 * @param text The text to render.
 * @return The same text as bytes, with a tab shown as "\t".
 */
static std::string ascii(const std::u16string_view text) {
    auto rendered = std::string{};
    for (const auto character : text) {
        if (character == u'\t') {
            rendered.append("\\t");
        } else {
            rendered.push_back(static_cast<char>(character));
        }
    }

    return rendered;
}

/**
 * @brief Visits every string up to a given length over an alphabet.
 *
 * Exhausting a small alphabet is what covers the sequences nobody thinks to write down — a quote
 * opened inside a word that is never closed, a run of spaces inside quotes, and so on.
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


TEST_CASE("a blank command line yields no token at all") {
    // ApplicationWindow::runCommand dispatches whatever comes back, so an empty prompt line has to
    // produce nothing rather than one empty command name
    CHECK(tokensOf(u"").empty());
    CHECK(tokensOf(u" ").empty());
    CHECK(tokensOf(u"     ").empty());
}

TEST_CASE("spaces separate tokens without ever producing an empty one") {
    CHECK(tokensOf(u"quit") == std::vector<std::u16string>{ u"quit" });
    CHECK(tokensOf(u"a b") == std::vector<std::u16string>{ u"a", u"b" });
    CHECK(tokensOf(u"a  b") == std::vector<std::u16string>{ u"a", u"b" });
    CHECK(tokensOf(u"  a  b  ") == std::vector<std::u16string>{ u"a", u"b" });
}

TEST_CASE("a tab is not a separator") {
    // Only U' ' splits, which matters for a script line indented with tabs: the indent would become
    // part of the command name
    CHECK(tokensOf(u"a\tb") == std::vector<std::u16string>{ u"a\tb" });
    CHECK(tokensOf(u"\tquit") == std::vector<std::u16string>{ u"\tquit" });
}

TEST_CASE("a quoted argument keeps its spaces") {
    // This is what quoting is for: open "my file.txt" has to reach OpenFileCommand as one argument
    CHECK(tokensOf(u"open \"a b\"") == std::vector<std::u16string>{ u"open", u"a b" });
    CHECK(tokensOf(u"open \"a  b\"") == std::vector<std::u16string>{ u"open", u"a  b" });
    CHECK(tokensOf(u"\"a b\" \"c d\"") == std::vector<std::u16string>{ u"a b", u"c d" });
}

TEST_CASE("an empty quoted argument is still an argument") {
    // open "" reaches the command with args.size() == 1 and an empty path, not with no argument
    CHECK(tokensOf(u"open \"\"") == std::vector<std::u16string>{ u"open", u"" });

    // A quote alone is an unterminated empty one, so the line dispatches with a blank command name
    CHECK(tokensOf(u"\"") == std::vector<std::u16string>{ u"" });
}

TEST_CASE("an unterminated quote takes the rest of the line") {
    CHECK(tokensOf(u"open \"a b") == std::vector<std::u16string>{ u"open", u"a b" });
    CHECK(tokensOf(u"open \"") == std::vector<std::u16string>{ u"open", u"" });
}

TEST_CASE("a quote is only special where a token starts") {
    // Inside a word the quote is an ordinary character, and both halves keep the one they carry
    CHECK(tokensOf(u"ab\"cd ef\"") == std::vector<std::u16string>{ u"ab\"cd", u"ef\"" });
    CHECK(tokensOf(u"ab\"cd\"") == std::vector<std::u16string>{ u"ab\"cd\"" });
}

TEST_CASE("text touching a closing quote starts a new token") {
    // The surprising one, and the reason to pin it: the two halves do not merge
    CHECK(tokensOf(u"\"ab\"cd") == std::vector<std::u16string>{ u"ab", u"cd" });
    CHECK(tokensOf(u"\"a b\"c d") == std::vector<std::u16string>{ u"a b", u"c", u"d" });
}

TEST_CASE("the tokens are views into the line they were parsed from") {
    // AutoCompleteCommand measures the argument being completed by subtracting the input's own
    // pointer from a token's, so a token that stopped aliasing the input would corrupt the prefix
    // it rebuilds rather than fail to compile
    constexpr auto input = std::u16string_view(u"cvar col_border 1");

    auto tokens = std::vector<std::u16string_view>{};
    CommandLine::tokenize(input, tokens);

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].data() == input.data());
    CHECK(tokens[1].data() == input.data() + 5);
    CHECK(tokens[2].data() == input.data() + 16);

    // A quoted token points past its opening quote, at the content itself
    constexpr auto quoted_input = std::u16string_view(u"open \"a b\"");

    CommandLine::tokenize(quoted_input, tokens);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[1].data() == quoted_input.data() + 6);
}

TEST_CASE("tokenizing clears whatever the caller's vector held") {
    // ApplicationWindow keeps one scratch vector across commands, so the leftovers of the previous
    // line have to go — including when the new line produces nothing
    auto tokens = std::vector<std::u16string_view>{ u"stale", u"leftovers", u"here" };

    CommandLine::tokenize(u"", tokens);
    CHECK(tokens.empty());

    CommandLine::tokenize(u"quit", tokens);
    REQUIRE(tokens.size() == 1);
    CHECK(std::u16string(tokens[0]) == std::u16string(u"quit"));
}

TEST_CASE("splitting a modifier combo yields one part per modifier") {
    CHECK(partsOf(u"Ctrl", u'+') == std::vector<std::u16string>{ u"Ctrl" });
    CHECK(partsOf(u"Ctrl+Shift", u'+') == std::vector<std::u16string>{ u"Ctrl", u"Shift" });
    CHECK(partsOf(u"Ctrl+Alt+Shift", u'+') == std::vector<std::u16string>{ u"Ctrl", u"Alt", u"Shift" });
}

TEST_CASE("splitting never produces an empty part") {
    // BindCommand maps each part to a modifier bit, so an empty part would be an unknown modifier
    CHECK(partsOf(u"Ctrl++Shift", u'+') == std::vector<std::u16string>{ u"Ctrl", u"Shift" });
    CHECK(partsOf(u"+Ctrl+", u'+') == std::vector<std::u16string>{ u"Ctrl" });
    CHECK(partsOf(u"", u'+').empty());

    // Nothing but delimiters means no modifier at all: bind + <key> <command> binds the bare key
    // and says nothing about it
    CHECK(partsOf(u"+", u'+').empty());
    CHECK(partsOf(u"+++", u'+').empty());
    CHECK(partsOf(u"aaa", u'a').empty());
}

TEST_CASE("the parts are views into the text they were split from") {
    constexpr auto input = std::u16string_view(u"Ctrl+Shift");

    const auto parts = CommandLine::split(input, u'+');

    REQUIRE(parts.size() == 2);
    CHECK(parts[0].data() == input.data());
    CHECK(parts[1].data() == input.data() + 5);
}

TEST_CASE("tokenizing agrees with a plain character-by-character reading of the rules") {
    // Every string of up to six characters over the four that mean something to the tokenizer: a
    // letter, the separator, the quote, and a tab that must stay ordinary
    auto mismatches = 0;
    const auto visited = forEachString(u"a \"\t", 6, [&mismatches](const std::u16string_view input) {
        const auto actual = tokensOf(input);
        const auto expected = referenceTokenize(input);
        if (actual != expected) {
            ++mismatches;
            if (mismatches <= 5) {
                const auto rendered = ascii(input);
                CAPTURE(rendered);
                CHECK(actual == expected);
            }
        }
    });

    CHECK(visited == 5461);
    CHECK(mismatches == 0);
}

TEST_CASE("splitting agrees with the same plain reading") {
    auto mismatches = 0;
    const auto visited = forEachString(u"ab+", 7, [&mismatches](const std::u16string_view input) {
        const auto actual = partsOf(input, u'+');
        const auto expected = referenceSplit(input, u'+');
        if (actual != expected) {
            ++mismatches;
            if (mismatches <= 5) {
                const auto rendered = ascii(input);
                CAPTURE(rendered);
                CHECK(actual == expected);
            }
        }
    });

    CHECK(visited == 3280);
    CHECK(mismatches == 0);
}
