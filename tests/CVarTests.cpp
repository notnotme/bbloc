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
#include <initializer_list>
#include <optional>
#include <vector>

#include "TestSupport.h"

#include "core/cvar/CVarBool.h"
#include "core/cvar/CVarColor.h"
#include "core/cvar/CVarFloat.h"
#include "core/cvar/CVarInt.h"


/**
 * @brief Sets a CVar the way CVarCommand does, from the tokens the prompt parsed.
 *
 * The arguments arrive as a span of views over the command line, so the tests hand over the same
 * thing: views into the literals below, never owning strings the CVar could keep.
 *
 * @param cvar The CVar to set.
 * @param args The value arguments, without the CVar name.
 * @return The error message, or nullopt when the value was accepted.
 */
static std::optional<std::u16string> setValue(CVar &cvar, const std::initializer_list<std::u16string_view> args) {
    const auto argument_list = std::vector<std::u16string_view>(args);
    return cvar.setValueFromStrings(argument_list);
}

/**
 * @brief Reports whether the CVar accepted the arguments.
 *
 * @param cvar The CVar to set.
 * @param args The value arguments, without the CVar name.
 * @return true when no message came back.
 */
static bool accepts(CVar &cvar, const std::initializer_list<std::u16string_view> args) {
    return !setValue(cvar, args).has_value();
}

/**
 * @brief Reports whether the CVar rejected the arguments and kept the value it already had.
 *
 * A rejection is two facts, not one: the prompt gets a message to show, and the CVar is left where
 * it was. The second is the one that matters at startup, where the theme scripts set every colour
 * and dimension and nobody is reading messages.
 *
 * @param cvar The CVar to set.
 * @param args The value arguments, without the CVar name.
 * @return true when a non-empty message came back and the value did not move.
 */
static bool rejects(CVar &cvar, const std::initializer_list<std::u16string_view> args) {
    const auto value_before = cvar.getStringValue();
    const auto message = setValue(cvar, args);
    return message.has_value() && !message->empty() && cvar.getStringValue() == value_before;
}

/**
 * @brief Collects the completion candidates a CVar offers for one component of its value.
 *
 * @param cvar The CVar to complete.
 * @param componentIndex The index of the value component being completed.
 * @return The candidates, in the order they were emitted.
 */
static std::vector<std::u16string> completions(const CVar &cvar, const int32_t componentIndex) {
    auto items = std::vector<std::u16string>{};
    cvar.provideValueCompletion(componentIndex, [&items](const std::u16string_view item) {
        items.emplace_back(item);
    });
    return items;
}


TEST_CASE("an int cvar round-trips through its string form") {
    auto cvar = CVarInt(42, false);

    CHECK(cvar.getStringValue() == std::u16string(u"42"));

    CHECK(accepts(cvar, { u"1234" }));
    CHECK(cvar.getStringValue() == std::u16string(u"1234"));

    CHECK(accepts(cvar, { u"-8" }));
    CHECK(cvar.getStringValue() == std::u16string(u"-8"));

    CHECK(accepts(cvar, { u"0" }));
    CHECK(cvar.getStringValue() == std::u16string(u"0"));
}

TEST_CASE("an int cvar rejects anything that is not exactly a decimal number") {
    auto cvar = CVarInt(42, false);

    // A number the parser stops inside is not a number: "4x" must not quietly become 4
    CHECK(rejects(cvar, { u"4x" }));
    CHECK(rejects(cvar, { u"12.5" }));

    // Base ten only, so a hex literal stops at its "0"
    CHECK(rejects(cvar, { u"0x10" }));

    CHECK(rejects(cvar, { u"" }));
    CHECK(rejects(cvar, { u"twelve" }));
}

TEST_CASE("an int cvar rejects a number too large to hold") {
    auto cvar = CVarInt(42, false);

    // Out of int32 range: the conversion throws, and the value has to survive it
    CHECK(rejects(cvar, { u"99999999999" }));
    CHECK(rejects(cvar, { u"-99999999999" }));
}

TEST_CASE("a cvar rejects the wrong number of arguments") {
    auto int_cvar = CVarInt(42, false);
    auto bool_cvar = CVarBool(true, false);
    auto float_cvar = CVarFloat(1.0f, false);
    auto color_cvar = CVarColor(1, 2, 3, 4, false);

    // The scalar types take exactly one argument
    CHECK(rejects(int_cvar, {}));
    CHECK(rejects(int_cvar, { u"1", u"2" }));
    CHECK(rejects(bool_cvar, {}));
    CHECK(rejects(bool_cvar, { u"true", u"true" }));
    CHECK(rejects(float_cvar, {}));
    CHECK(rejects(float_cvar, { u"1.0", u"2.0" }));

    // A colour takes three or four
    CHECK(rejects(color_cvar, {}));
    CHECK(rejects(color_cvar, { u"10", u"20" }));
    CHECK(rejects(color_cvar, { u"10", u"20", u"30", u"40", u"50" }));
}

TEST_CASE("the read-only flag is not the value setter's business") {
    auto cvar = CVarInt(7, true);

    CHECK(cvar.isReadOnly());

    // Read-only is enforced by CVarCommand before it ever calls down here, which is the documented
    // contract: setValueFromStrings must not second-guess it
    CHECK(accepts(cvar, { u"9" }));
    CHECK(cvar.getStringValue() == std::u16string(u"9"));
}

TEST_CASE("an int cvar completes with the value it holds") {
    const auto cvar = CVarInt(42, false);

    const auto first_component = completions(cvar, 0);
    REQUIRE(first_component.size() == 1);
    CHECK(first_component[0] == std::u16string(u"42"));

    // A single-component value has nothing to offer past the first
    CHECK(completions(cvar, 1).empty());
}

TEST_CASE("a bool cvar reads and writes the two words it knows") {
    auto cvar = CVarBool(true, false);

    CHECK(cvar.getStringValue() == std::u16string(u"true"));

    CHECK(accepts(cvar, { u"false" }));
    CHECK(cvar.getStringValue() == std::u16string(u"false"));

    CHECK(accepts(cvar, { u"true" }));
    CHECK(cvar.getStringValue() == std::u16string(u"true"));
}

TEST_CASE("a bool cvar rejects every other spelling of a boolean") {
    auto cvar = CVarBool(true, false);

    // The scripts are typed by hand, so the near-misses are the ones worth naming
    CHECK(rejects(cvar, { u"True" }));
    CHECK(rejects(cvar, { u"TRUE" }));
    CHECK(rejects(cvar, { u"1" }));
    CHECK(rejects(cvar, { u"0" }));
    CHECK(rejects(cvar, { u"yes" }));
    CHECK(rejects(cvar, { u"" }));
}

TEST_CASE("a bool cvar offers both words whatever it currently holds") {
    const auto cvar = CVarBool(true, false);

    // Completing a boolean has to suggest the value it is not, so the default scanner — which only
    // ever emits the current value — is overridden here
    const auto candidates = completions(cvar, 0);
    REQUIRE(candidates.size() == 2);
    CHECK(candidates[0] == std::u16string(u"false"));
    CHECK(candidates[1] == std::u16string(u"true"));

    CHECK(completions(cvar, 1).empty());
}

TEST_CASE("a float cvar round-trips through the six decimals it renders with") {
    auto cvar = CVarFloat(1.5f, false);

    // std::to_string is what the prompt shows and what the completion offers back
    CHECK(cvar.getStringValue() == std::u16string(u"1.500000"));

    CHECK(accepts(cvar, { u"-0.25" }));
    CHECK(cvar.getStringValue() == std::u16string(u"-0.250000"));

    // An integer is a float too, and comes back as one
    CHECK(accepts(cvar, { u"3" }));
    CHECK(cvar.getStringValue() == std::u16string(u"3.000000"));
}

TEST_CASE("a float cvar rejects text the parser cannot finish") {
    auto cvar = CVarFloat(1.5f, false);

    CHECK(rejects(cvar, { u"4x" }));
    CHECK(rejects(cvar, { u"" }));
    CHECK(rejects(cvar, { u"one" }));

    // Beyond the range of a float, the conversion throws
    CHECK(rejects(cvar, { u"1e400" }));
}

TEST_CASE("a float cvar rejects a number that is not finite") {
    auto cvar = CVarFloat(1.5f, false);

    // These are the words the conversion accepts and consumes whole, so only an explicit check
    // stops them: a NaN or an infinity taken here spreads through every measure using the value
    CHECK(rejects(cvar, { u"nan" }));
    CHECK(rejects(cvar, { u"NaN" }));
    CHECK(rejects(cvar, { u"inf" }));
    CHECK(rejects(cvar, { u"-inf" }));
    CHECK(rejects(cvar, { u"infinity" }));
}

TEST_CASE("a colour cvar renders its four channels in order") {
    const auto cvar = CVarColor(10, 20, 30, 40, false);

    CHECK(cvar.getStringValue() == std::u16string(u"10 20 30 40"));
}

TEST_CASE("a colour cvar takes three channels and assumes an opaque alpha") {
    auto cvar = CVarColor(10, 20, 30, 40, false);

    // The theme scripts write colours both ways, and the three-channel form must not inherit the
    // alpha that happened to be there
    CHECK(accepts(cvar, { u"0", u"128", u"255" }));
    CHECK(cvar.getStringValue() == std::u16string(u"0 128 255 255"));

    CHECK(accepts(cvar, { u"1", u"2", u"3", u"4" }));
    CHECK(cvar.getStringValue() == std::u16string(u"1 2 3 4"));
}

TEST_CASE("a colour cvar rejects a channel outside a byte") {
    auto cvar = CVarColor(10, 20, 30, 40, false);

    CHECK(rejects(cvar, { u"256", u"0", u"0" }));
    CHECK(rejects(cvar, { u"-1", u"0", u"0" }));
    CHECK(rejects(cvar, { u"12.5", u"0", u"0" }));
    CHECK(rejects(cvar, { u"", u"0", u"0" }));
    CHECK(rejects(cvar, { u"0", u"0", u"0", u"300" }));
}

TEST_CASE("a colour cvar keeps its old colour when a later channel is bad") {
    auto cvar = CVarColor(10, 20, 30, 40, false);

    // The channels are parsed before any of them is stored, so a colour never ends up half applied
    CHECK(rejects(cvar, { u"1", u"2", u"300" }));
    CHECK(cvar.getStringValue() == std::u16string(u"10 20 30 40"));
}

TEST_CASE("a colour cvar completes one channel at a time") {
    const auto cvar = CVarColor(10, 20, 30, 40, false);

    // Four components read off the rendered value by the default scanner in CVar
    const auto expected = std::vector<std::u16string>{ u"10", u"20", u"30", u"40" };
    for (auto index = 0; index < 4; ++index) {
        const auto candidates = completions(cvar, index);
        REQUIRE(candidates.size() == 1);
        CHECK(candidates[0] == expected[static_cast<std::size_t>(index)]);
    }

    // A colour has no fifth component
    CHECK(completions(cvar, 4).empty());
}
