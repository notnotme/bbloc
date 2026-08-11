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
#include "KeyModifiers.h"

#include <ranges>

#include "PadInput.h"


const U16StringMap<uint16_t> KeyModifiers::MODIFIER_MAP = {
    { u"Ctrl", KMOD_CTRL },
    { u"Shift", KMOD_SHIFT },
    { u"Alt", KMOD_ALT },
    { u"L", PadInput::KMOD_PAD_L },
    { u"R", PadInput::KMOD_PAD_R },
    { u"None", KMOD_NONE }
};

uint16_t KeyModifiers::normalize(const uint16_t modifiers) {
    auto result = 0;
    if (modifiers & (KMOD_LCTRL | KMOD_RCTRL)) {
        result |= KMOD_CTRL;
    }

    if (modifiers & (KMOD_LSHIFT | KMOD_RSHIFT)) {
        result |= KMOD_SHIFT;
    }

    if (modifiers & (KMOD_LALT | KMOD_RALT)) {
        result |= KMOD_ALT;
    }

    if (modifiers & (KMOD_LGUI | KMOD_RGUI)) {
        result |= KMOD_GUI;
    }

    // The pad modifier bits (held shoulders) have no left/right variants: pass them through.
    result |= modifiers & (PadInput::KMOD_PAD_L | PadInput::KMOD_PAD_R);

    return result;
}

int32_t KeyModifiers::fromName(const std::u16string_view modifier) {
    if (const auto &mapped_modifier = MODIFIER_MAP.find(modifier); mapped_modifier != MODIFIER_MAP.end()) {
        return mapped_modifier->second;
    }

    return -1;
}

void KeyModifiers::forEachName(const AutoCompleteCallback &itemCallback) {
    for (const auto &name : std::views::keys(MODIFIER_MAP)) {
        itemCallback(name);
    }
}
