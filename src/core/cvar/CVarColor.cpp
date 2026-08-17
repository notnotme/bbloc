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
#include "../cvar/CVarColor.h"

#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <utf8.h>


/** @brief Parses a color channel in [0, 255]; returns nullopt on any other input. */
static std::optional<uint8_t> parseChannel(const std::u16string_view arg) {
    const auto utf8_arg = utf8::utf16to8(arg);
    auto value = int32_t{0};
    const auto [ptr, ec] = std::from_chars(utf8_arg.data(), utf8_arg.data() + utf8_arg.length(), value);
    if (ec != std::errc{} || ptr != utf8_arg.data() + utf8_arg.length() || value < 0 || value > 255) {
        return std::nullopt;
    }

    return static_cast<uint8_t>(value);
}

CVarColor::CVarColor(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha, const bool isReadOnly)
    : TypedCVar(Color{.red = red, .green = green, .blue = blue, .alpha = alpha}, isReadOnly) {}

std::optional<std::u16string> CVarColor::setValueFromStrings(const std::span<const std::u16string_view> args) {
    if (args.size() < 3 || args.size() > 4) {
        return u"Argument expected: <red> <green> <blue> [alpha].";
    }

    const auto arg_r = parseChannel(args[0]);
    const auto arg_g = parseChannel(args[1]);
    const auto arg_b = parseChannel(args[2]);
    const auto arg_a = args.size() >= 4 ? parseChannel(args[3]) : std::optional<uint8_t>{255};
    if (!arg_r.has_value() || !arg_g.has_value() || !arg_b.has_value() || !arg_a.has_value()) {
        return u"Unable to convert arguments to color";
    }

    m_value = Color{.red = *arg_r, .green = *arg_g, .blue = *arg_b, .alpha = *arg_a};
    return std::nullopt;
}

std::u16string CVarColor::getStringValue() const {
    return utf8::utf8to16(std::format("{} {} {} {}",
        static_cast<uint32_t>(m_value.red), static_cast<uint32_t>(m_value.green),
        static_cast<uint32_t>(m_value.blue), static_cast<uint32_t>(m_value.alpha)));
}
