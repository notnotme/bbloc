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
#ifndef PAD_INPUT_H
#define PAD_INPUT_H

#include <string_view>

#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>

#include "AutoCompleteCallback.h"


/**
 * @brief Static-only mapping of game-controller inputs into the SDL_Keycode space used by the key bindings.
 *
 * SDL_Keycode is a signed 32-bit type and every real keycode is positive, so negative values
 * are collision-free: controller buttons and axis directions are encoded as negative
 * pseudo-keycodes and flow through the exact same binding path as keyboard keys. The two
 * shoulders are not bindable inputs: they act as the L/R binding modifiers, carried in the
 * two KMOD bits SDL leaves free.
 *
 * Binding names use a "pad:" prefix followed by the SDL game-controller string, e.g. "pad:a",
 * "pad:dpup", "pad:lefty-" (stick axes need a '-'/'+' direction suffix) or "pad:lefttrigger"
 * (triggers only go positive, the suffix is optional).
 */
class PadInput final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    PadInput() = delete;

    /** Modifier bit for a held left shoulder; one of the two KMOD bits SDL leaves free. */
    static constexpr uint16_t KMOD_PAD_L = 0x0004;

    /** Modifier bit for a held right shoulder; one of the two KMOD bits SDL leaves free. */
    static constexpr uint16_t KMOD_PAD_R = 0x0008;

    /**
     * @brief Encodes a controller button as a negative pseudo-keycode.
     *
     * @param button The SDL controller button to encode.
     * @return The negative pseudo-keycode of the button.
     */
    [[nodiscard]] static constexpr SDL_Keycode fromButton(const SDL_GameControllerButton button) {
        return -(1 + static_cast<SDL_Keycode>(button));
    }

    /**
     * @brief Encodes one direction of a controller axis as a negative pseudo-keycode.
     *
     * The base offset keeps the axis range disjoint from the button range.
     *
     * @param axis The SDL controller axis to encode.
     * @param positive True for the positive direction of the axis, false for the negative one.
     * @return The negative pseudo-keycode of the axis direction.
     */
    [[nodiscard]] static constexpr SDL_Keycode fromAxis(const SDL_GameControllerAxis axis, const bool positive) {
        return -(64 + 2 * static_cast<SDL_Keycode>(axis) + (positive ? 1 : 0));
    }

    /**
     * @brief Resolves a "pad:..." binding name into its pad pseudo-keycode.
     *
     * Buttons resolve through SDL_GameControllerGetButtonFromString, rejecting the shoulders
     * (reserved as the L/R binding modifiers). Stick axes require a '-'/'+' direction suffix;
     * triggers only go positive, so their suffix is optional and '-' is rejected.
     *
     * @param name The binding name, expected to start with "pad:".
     * @return The pad pseudo-keycode, or SDLK_UNKNOWN when the name does not resolve.
     */
    [[nodiscard]] static SDL_Keycode keycodeFromName(std::u16string_view name);

    /**
     * @brief Enumerates every bindable "pad:..." name, for auto-completion.
     *
     * Emits the buttons (without the shoulders), both directions of the stick axes, and the
     * triggers without a suffix.
     *
     * @param itemCallback A callback invoked with each name.
     */
    static void forEachName(const AutoCompleteCallback &itemCallback);
};


#endif //PAD_INPUT_H
