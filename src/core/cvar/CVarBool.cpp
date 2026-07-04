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
#include "../cvar/CVarBool.h"

#include <string>
#include <utf8.h>


CVarBool::CVarBool(const bool value, const bool isReadOnly)
    : TypedCVar(value, isReadOnly) {}

std::u16string CVarBool::getStringValue() const {
    return m_value ? u"true" : u"false";
}

void CVarBool::provideValueCompletion(const int32_t componentIndex, const AutoCompleteCallback &itemCallback) const {
    if (componentIndex != 0) {
        // A boolean has a single component
        return;
    }

    itemCallback(u"false");
    itemCallback(u"true");
}

std::optional<std::u16string> CVarBool::setValueFromStrings(const std::span<const std::u16string_view> args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    if (args[0] == u"true") {
        m_value = true;
    } else if (args[0] == u"false") {
        m_value = false;
    } else {
        return u"Unable to convert argument to boolean";
    }

    return std::nullopt;
}
