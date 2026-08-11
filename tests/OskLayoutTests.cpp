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
#include <string>
#include <string_view>
#include <vector>

#include "TestSupport.h"

#include "osk/OskLayout.h"


namespace {
    /** The nine known layout names, in table order. */
    constexpr std::string_view LAYOUT_NAMES[] = {
        "qwerty", "azerty", "qwertz", "uk", "spanish", "spanish_latin", "italian", "portuguese", "russian"
    };

    /** The number of scancode slots the base table covers. */
    constexpr std::size_t SCANCODE_COUNT = 256;

    /**
     * @brief Resolves a slot and materialises the result, so a failing CHECK shows the text.
     *
     * @param layout The layout to resolve in.
     * @param scancode The key slot.
     * @param shift True when Shift applies.
     * @param altgr True when AltGr applies.
     * @return The resolved UTF-8 string, or "<null>" when the slot resolves to nothing.
     */
    std::string form(const OskLayout::Layout &layout, const SDL_Scancode scancode, const bool shift, const bool altgr) {
        const auto *resolved = OskLayout::resolve(layout, scancode, shift, altgr);
        return resolved == nullptr ? std::string{"<null>"} : std::string{resolved};
    }

    /**
     * @brief Finds a layout a case requires to exist.
     *
     * @param name The layout name.
     * @return The layout.
     */
    const OskLayout::Layout &layoutOf(const std::string_view name) {
        const auto *layout = OskLayout::findLayout(name);
        REQUIRE(layout != nullptr);

        return *layout;
    }
}


TEST_CASE("every known layout name finds its layout") {
    for (const auto name : LAYOUT_NAMES) {
        CAPTURE(name);
        const auto *layout = OskLayout::findLayout(name);

        REQUIRE(layout != nullptr);
        CHECK(layout->name == name);
    }
}

TEST_CASE("lookalike and empty names find nothing") {
    CHECK(OskLayout::findLayout("colemak") == nullptr);
    CHECK(OskLayout::findLayout("qwert") == nullptr);      // a prefix is not a match
    CHECK(OskLayout::findLayout("QWERTY") == nullptr);     // neither is a different case
    CHECK(OskLayout::findLayout("") == nullptr);
}

TEST_CASE("the default layout is qwerty, with no override and no accent") {
    const auto &layout = OskLayout::defaultLayout();

    CHECK(&layout == OskLayout::findLayout("qwerty"));
    CHECK(std::string_view(layout.name) == "qwerty");
    CHECK(layout.letters.empty());
    CHECK(layout.accents.empty());
}

TEST_CASE("the us base gives letters their two cases") {
    const auto &qwerty = OskLayout::defaultLayout();

    CHECK(form(qwerty, SDL_SCANCODE_A, false, false) == "a");
    CHECK(form(qwerty, SDL_SCANCODE_A, true, false) == "A");
    CHECK(form(qwerty, SDL_SCANCODE_Q, false, false) == "q");
    CHECK(form(qwerty, SDL_SCANCODE_Q, true, false) == "Q");
    CHECK(form(qwerty, SDL_SCANCODE_Z, false, false) == "z");
    CHECK(form(qwerty, SDL_SCANCODE_Z, true, false) == "Z");
}

TEST_CASE("the us base keeps digits plain and shifts them to the us symbols") {
    const auto &qwerty = OskLayout::defaultLayout();

    CHECK(form(qwerty, SDL_SCANCODE_1, false, false) == "1");
    CHECK(form(qwerty, SDL_SCANCODE_1, true, false) == "!");
    CHECK(form(qwerty, SDL_SCANCODE_0, false, false) == "0");
    CHECK(form(qwerty, SDL_SCANCODE_0, true, false) == ")");
}

TEST_CASE("a slot the base does not define resolves to nothing") {
    const auto &qwerty = OskLayout::defaultLayout();

    CHECK(OskLayout::resolve(qwerty, SDL_SCANCODE_F1, false, false) == nullptr);
    CHECK(OskLayout::resolve(qwerty, SDL_SCANCODE_F1, true, false) == nullptr);
    CHECK(OskLayout::resolve(qwerty, SDL_SCANCODE_F1, false, true) == nullptr);
    CHECK(OskLayout::resolve(qwerty, SDL_SCANCODE_F1, true, true) == nullptr);
}

TEST_CASE("azerty permutes the letters without losing any pair") {
    const auto &azerty = layoutOf("azerty");

    CHECK(form(azerty, SDL_SCANCODE_Q, false, false) == "a");
    CHECK(form(azerty, SDL_SCANCODE_Q, true, false) == "A");
    CHECK(form(azerty, SDL_SCANCODE_A, false, false) == "q");
    CHECK(form(azerty, SDL_SCANCODE_A, true, false) == "Q");
    CHECK(form(azerty, SDL_SCANCODE_SEMICOLON, false, false) == "m");
    CHECK(form(azerty, SDL_SCANCODE_SEMICOLON, true, false) == "M");
    CHECK(form(azerty, SDL_SCANCODE_M, false, false) == ";");   // the freed slot keeps the semicolon pair
    CHECK(form(azerty, SDL_SCANCODE_M, true, false) == ":");
}

TEST_CASE("qwertz swaps y and z") {
    const auto &qwertz = layoutOf("qwertz");

    CHECK(form(qwertz, SDL_SCANCODE_Y, false, false) == "z");
    CHECK(form(qwertz, SDL_SCANCODE_Y, true, false) == "Z");
    CHECK(form(qwertz, SDL_SCANCODE_Z, false, false) == "y");
    CHECK(form(qwertz, SDL_SCANCODE_Z, true, false) == "Y");
}

TEST_CASE("russian replaces the letter zone and spills onto the punctuation slots") {
    const auto &russian = layoutOf("russian");

    CHECK(form(russian, SDL_SCANCODE_Q, false, false) == "й");
    CHECK(form(russian, SDL_SCANCODE_Q, true, false) == "Й");
    CHECK(form(russian, SDL_SCANCODE_SEMICOLON, false, false) == "ж");
    CHECK(form(russian, SDL_SCANCODE_SEMICOLON, true, false) == "Ж");
    CHECK(form(russian, SDL_SCANCODE_SLASH, false, false) == "ё");
    CHECK(form(russian, SDL_SCANCODE_SLASH, true, false) == "Ё");
}

TEST_CASE("accents attach to the displayed letter, not to the slot") {
    const auto &azerty = layoutOf("azerty");

    // The Q slot displays a, so it carries the a accent
    CHECK(form(azerty, SDL_SCANCODE_Q, false, true) == "à");
    CHECK(form(azerty, SDL_SCANCODE_Q, true, true) == "À");

    // The A slot displays q, so it carries the œ ligature the free q was given
    CHECK(form(azerty, SDL_SCANCODE_A, false, true) == "œ");
    CHECK(form(azerty, SDL_SCANCODE_A, true, true) == "Œ");

    // The E slot is not permuted; its accent is the plain mnemonic one
    CHECK(form(azerty, SDL_SCANCODE_E, false, true) == "é");
    CHECK(form(azerty, SDL_SCANCODE_E, true, true) == "É");
}

TEST_CASE("a letter with no accent keeps its plain column under altgr") {
    const auto &azerty = layoutOf("azerty");

    CHECK(form(azerty, SDL_SCANCODE_B, false, true) == "b");
    CHECK(form(azerty, SDL_SCANCODE_B, true, true) == "B");
}

TEST_CASE("a layout with no accent map resolves altgr to the plain letter") {
    const auto &russian = layoutOf("russian");

    CHECK(form(russian, SDL_SCANCODE_Q, false, true) == "й");
    CHECK(form(russian, SDL_SCANCODE_Q, true, true) == "Й");
}

TEST_CASE("forEachName enumerates exactly the nine names, in table order") {
    auto names = std::vector<std::u16string>{};
    OskLayout::forEachName([&names](const std::u16string_view name) {
        names.emplace_back(name);
    });

    auto expected = std::vector<std::u16string>{};
    for (const auto name : LAYOUT_NAMES) {
        expected.emplace_back(name.begin(), name.end());
    }

    CHECK(names == expected);
}

TEST_CASE("a slot with a plain form never blanks out under any modifier combination") {
    for (const auto name : LAYOUT_NAMES) {
        const auto &layout = layoutOf(name);

        for (auto code = std::size_t{0}; code < SCANCODE_COUNT; ++code) {
            const auto scancode = static_cast<SDL_Scancode>(code);
            if (OskLayout::resolve(layout, scancode, false, false) == nullptr) {
                continue;
            }
            CAPTURE(name);
            CAPTURE(code);

            CHECK(OskLayout::resolve(layout, scancode, true, false) != nullptr);
            CHECK(OskLayout::resolve(layout, scancode, false, true) != nullptr);
            CHECK(OskLayout::resolve(layout, scancode, true, true) != nullptr);
        }
    }
}

TEST_CASE("digits resolve to themselves in every layout") {
    constexpr struct { SDL_Scancode scancode; const char *digit; } DIGITS[] = {
        { SDL_SCANCODE_1, "1" }, { SDL_SCANCODE_2, "2" }, { SDL_SCANCODE_3, "3" },
        { SDL_SCANCODE_4, "4" }, { SDL_SCANCODE_5, "5" }, { SDL_SCANCODE_6, "6" },
        { SDL_SCANCODE_7, "7" }, { SDL_SCANCODE_8, "8" }, { SDL_SCANCODE_9, "9" },
        { SDL_SCANCODE_0, "0" }
    };

    for (const auto name : LAYOUT_NAMES) {
        const auto &layout = layoutOf(name);

        for (const auto &[scancode, digit] : DIGITS) {
            CAPTURE(name);
            CAPTURE(digit);

            CHECK(form(layout, scancode, false, false) == digit);
            CHECK(form(layout, scancode, false, true) == digit);    // no accent ever lands on a digit
        }
    }
}

TEST_CASE("every accent entry is carried by a displayed letter of its layout") {
    for (const auto name : LAYOUT_NAMES) {
        const auto &layout = layoutOf(name);

        for (const auto &accent : layout.accents) {
            CAPTURE(name);
            CAPTURE(accent.letter);

            auto displayed = false;
            for (auto code = std::size_t{0}; code < SCANCODE_COUNT && !displayed; ++code) {
                const auto *plain = OskLayout::resolve(layout, static_cast<SDL_Scancode>(code), false, false);
                displayed = plain != nullptr && std::string_view(plain) == accent.letter;
            }

            CHECK(displayed);   // a dead accent entry is one missing character on one layout
        }
    }
}
