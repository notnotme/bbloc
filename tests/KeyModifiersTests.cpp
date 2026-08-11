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
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <SDL_keycode.h>

#include "TestSupport.h"

#include "core/base/KeyModifiers.h"
#include "core/base/PadInput.h"


namespace {
    /** The six modifier names the map holds, with the bits each one stands for. */
    constexpr struct { std::u16string_view name; uint16_t bits; } NAMED_MODIFIERS[] = {
        { u"Ctrl", KMOD_CTRL },
        { u"Shift", KMOD_SHIFT },
        { u"Alt", KMOD_ALT },
        { u"L", PadInput::KMOD_PAD_L },
        { u"R", PadInput::KMOD_PAD_R },
        { u"None", KMOD_NONE }
    };
}


TEST_CASE("each left and right variant folds to its pair bit") {
    constexpr struct { uint16_t left; uint16_t right; uint16_t pair; } VARIANTS[] = {
        { KMOD_LCTRL, KMOD_RCTRL, KMOD_CTRL },
        { KMOD_LSHIFT, KMOD_RSHIFT, KMOD_SHIFT },
        { KMOD_LALT, KMOD_RALT, KMOD_ALT },
        { KMOD_LGUI, KMOD_RGUI, KMOD_GUI }
    };

    for (const auto &[left, right, pair] : VARIANTS) {
        CAPTURE(pair);

        CHECK(KeyModifiers::normalize(left) == pair);
        CHECK(KeyModifiers::normalize(right) == pair);
        CHECK(KeyModifiers::normalize(left | right) == pair);   // both held is still one chord
    }
}

TEST_CASE("modifiers fold independently in combination") {
    CHECK(KeyModifiers::normalize(KMOD_LCTRL | KMOD_LSHIFT) == (KMOD_CTRL | KMOD_SHIFT));
    CHECK(KeyModifiers::normalize(KMOD_RCTRL | KMOD_LSHIFT | KMOD_RALT) == (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT));
    CHECK(KeyModifiers::normalize(KMOD_LCTRL | KMOD_RSHIFT | KMOD_LALT | KMOD_RGUI)
          == (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT | KMOD_GUI));
}

TEST_CASE("no modifier normalizes to zero") {
    CHECK(KeyModifiers::normalize(KMOD_NONE) == 0);
}

TEST_CASE("the pad bits pass through, alone and beside a keyboard modifier") {
    CHECK(KeyModifiers::normalize(PadInput::KMOD_PAD_L) == PadInput::KMOD_PAD_L);
    CHECK(KeyModifiers::normalize(PadInput::KMOD_PAD_R) == PadInput::KMOD_PAD_R);
    CHECK(KeyModifiers::normalize(PadInput::KMOD_PAD_L | PadInput::KMOD_PAD_R)
          == (PadInput::KMOD_PAD_L | PadInput::KMOD_PAD_R));

    CHECK(KeyModifiers::normalize(PadInput::KMOD_PAD_L | KMOD_LCTRL) == (PadInput::KMOD_PAD_L | KMOD_CTRL));
    CHECK(KeyModifiers::normalize(PadInput::KMOD_PAD_R | KMOD_RSHIFT) == (PadInput::KMOD_PAD_R | KMOD_SHIFT));
}

TEST_CASE("the lock keys and every other unhandled bit are dropped") {
    CHECK(KeyModifiers::normalize(KMOD_CAPS) == 0);
    CHECK(KeyModifiers::normalize(KMOD_NUM) == 0);
    CHECK(KeyModifiers::normalize(KMOD_MODE) == 0);
    CHECK(KeyModifiers::normalize(KMOD_SCROLL) == 0);

    // The load-bearing consequence: a binding still matches with a lock engaged
    CHECK(KeyModifiers::normalize(KMOD_LCTRL | KMOD_CAPS) == KeyModifiers::normalize(KMOD_LCTRL));
    CHECK(KeyModifiers::normalize(KMOD_LSHIFT | KMOD_NUM) == KeyModifiers::normalize(KMOD_LSHIFT));
}

TEST_CASE("normalize is idempotent across every 16-bit value") {
    auto all_idempotent = true;
    for (auto value = uint32_t{0}; value <= UINT16_MAX; ++value) {
        const auto once = KeyModifiers::normalize(static_cast<uint16_t>(value));
        if (once != KeyModifiers::normalize(once)) {
            all_idempotent = false;
            CAPTURE(value);
            CHECK(KeyModifiers::normalize(once) == once);
        }
    }

    CHECK(all_idempotent);
}

TEST_CASE("fromName maps the six names to their bits") {
    for (const auto &[name, bits] : NAMED_MODIFIERS) {
        CAPTURE(std::u16string{name});

        CHECK(KeyModifiers::fromName(name) == bits);
    }
}

TEST_CASE("fromName rejects unknown, wrong-case and empty names") {
    CHECK(KeyModifiers::fromName(u"Gui") == -1);    // GUI folds in normalize but has no binding name
    CHECK(KeyModifiers::fromName(u"Meta") == -1);
    CHECK(KeyModifiers::fromName(u"ctrl") == -1);
    CHECK(KeyModifiers::fromName(u"SHIFT") == -1);
    CHECK(KeyModifiers::fromName(u"Ctr") == -1);    // a prefix is not a match
    CHECK(KeyModifiers::fromName(u"") == -1);
}

TEST_CASE("forEachName enumerates exactly the six names") {
    auto names = std::vector<std::u16string>{};
    KeyModifiers::forEachName([&names](const std::u16string_view name) {
        names.emplace_back(name);
    });

    auto expected = std::vector<std::u16string>{};
    for (const auto &[name, bits] : NAMED_MODIFIERS) {
        expected.emplace_back(name);
    }

    // The map is unordered, so the names are compared as a set
    std::sort(names.begin(), names.end());
    std::sort(expected.begin(), expected.end());
    CHECK(names == expected);
}

TEST_CASE("every named modifier is a fixed point of normalize") {
    // A binding stores fromName's bits and matches against normalize's: the two must agree
    for (const auto &[name, bits] : NAMED_MODIFIERS) {
        CAPTURE(std::u16string{name});

        CHECK(KeyModifiers::normalize(bits) == bits);
    }
}
