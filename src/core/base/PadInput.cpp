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
#include "PadInput.h"

#include <string>

#include <utf8.h>


namespace {
    /** Prefix every pad binding name carries. */
    constexpr std::u16string_view NAME_PREFIX = u"pad:";

    /**
     * @brief Tells whether the axis is a trigger, which only ever goes positive.
     *
     * @param axis The SDL controller axis to test.
     * @return true when the axis is one of the two triggers.
     */
    constexpr bool isTrigger(const SDL_GameControllerAxis axis) {
        return axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    }
}

SDL_Keycode PadInput::keycodeFromName(const std::u16string_view name) {
    if (!name.starts_with(NAME_PREFIX)) {
        return SDLK_UNKNOWN;
    }

    // The SDL lookups work on UTF-8; pad names are plain ASCII either way.
    const auto body = utf8::utf16to8(name.substr(NAME_PREFIX.size()));

    // Try a button name first. The shoulders are reserved as the L/R binding modifiers.
    const auto button = SDL_GameControllerGetButtonFromString(body.c_str());
    if (button != SDL_CONTROLLER_BUTTON_INVALID) {
        if (button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER || button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
            return SDLK_UNKNOWN;
        }

        return fromButton(button);
    }

    // Axis: an optional trailing '-'/'+' selects the direction.
    const auto has_suffix = !body.empty() && (body.back() == '-' || body.back() == '+');
    const auto positive = !has_suffix || body.back() == '+';
    const auto axis_name = has_suffix ? body.substr(0, body.size() - 1) : body;
    const auto axis = SDL_GameControllerGetAxisFromString(axis_name.c_str());
    if (axis == SDL_CONTROLLER_AXIS_INVALID) {
        return SDLK_UNKNOWN;
    }

    if (isTrigger(axis)) {
        // Triggers only go positive: the suffix is optional and '-' does not resolve.
        return positive ? fromAxis(axis, true) : SDLK_UNKNOWN;
    }

    // Stick axes are bidirectional: the direction suffix is mandatory.
    if (!has_suffix) {
        return SDLK_UNKNOWN;
    }

    return fromAxis(axis, positive);
}

void PadInput::forEachName(const AutoCompleteCallback &itemCallback) {
    // Every button except the shoulders, which are the L/R binding modifiers.
    for (auto button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
        if (button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER || button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
            continue;
        }

        const auto *button_name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button));
        if (button_name == nullptr) {
            continue;
        }

        itemCallback(std::u16string(NAME_PREFIX).append(utf8::utf8to16(std::string_view(button_name))));
    }

    // Both directions of the stick axes; the triggers as their bare, suffix-less name.
    for (auto axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
        const auto *axis_name = SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(axis));
        if (axis_name == nullptr) {
            continue;
        }

        const auto name = std::u16string(NAME_PREFIX).append(utf8::utf8to16(std::string_view(axis_name)));
        if (isTrigger(static_cast<SDL_GameControllerAxis>(axis))) {
            itemCallback(name);
        } else {
            itemCallback(std::u16string(name).append(u"-"));
            itemCallback(std::u16string(name).append(u"+"));
        }
    }
}
