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
#include "../cvar/CVarInt.h"

#include <charconv>
#include <string>
#include <system_error>
#include <utf8.h>


CVarInt::CVarInt(const int32_t value, const bool isReadOnly)
    : TypedCVar(value, isReadOnly) {}

std::optional<std::u16string> CVarInt::setValueFromStrings(const std::span<const std::u16string_view> args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    const auto arg = utf8::utf16to8(args[0]);
    auto value = int32_t{0};
    const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.length(), value);
    if (ec != std::errc{} || ptr != arg.data() + arg.length()) {
        // The whole argument must convert: reject trailing garbage such as "4x"
        return u"Unable to convert argument to int";
    }

    m_value = value;
    return std::nullopt;
}

std::u16string CVarInt::getStringValue() const {
    return utf8::utf8to16(std::to_string(m_value));
}
