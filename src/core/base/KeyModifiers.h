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
#ifndef KEY_MODIFIERS_H
#define KEY_MODIFIERS_H

#include <string_view>

#include <SDL_keycode.h>

#include "AutoCompleteCallback.h"
#include "U16StringMap.h"


/**
 * @brief Static-only folding and naming of the key modifiers used by the key bindings.
 *
 * The modifier names ("Ctrl", "Shift", "Alt", "L", "R", "None") map to their KMOD bits for the
 * bind command, and raw SDL modifier state folds into the same bits so a binding matches
 * regardless of left/right variants or lock keys. The pad shoulder bits
 * (PadInput::KMOD_PAD_L / KMOD_PAD_R) pass through unchanged.
 */
class KeyModifiers final {
private:
    /** Lookup map to ease mapping modifiers. */
    static const U16StringMap<uint16_t> MODIFIER_MAP;

public:
    /** @brief Deleted constructor; this class is static-only. */
    KeyModifiers() = delete;

    /**
     * @brief Normalize input modifiers from raw SDL input modifiers.
     *
     * This basically converts SDL modifiers Left/Right to universal modifier position (LSHIFT -> SHIFT).
     * The pad modifier bits (KMOD_PAD_L / KMOD_PAD_R) pass through unchanged.
     *
     * @param modifiers The raw SDL modifier flags.
     * @return Normalized modifier flags.
     */
    [[nodiscard]] static uint16_t normalize(uint16_t modifiers);

    /**
     * @brief Map a string representation of a key modifier into a int32_t.
     *
     * This converts a SDL modifier keycode into an int32_t. This allows returning a negative value in case the mapping
     * fails, and return an error in this case.
     *
     * Values can be "Ctrl", "Alt", "Shift", "L", "R", "None".
     * "None" is mandatory if the binding does not use key modifiers at all.
     *
     * @param modifier The string representation of the modifier to convert.
     * @return An int representation of the modifier, or -1.
     */
    [[nodiscard]] static int32_t fromName(std::u16string_view modifier);

    /**
     * @brief Enumerates every modifier name, for auto-completion.
     *
     * @param itemCallback A callback invoked with each name.
     */
    static void forEachName(const AutoCompleteCallback &itemCallback);
};


#endif //KEY_MODIFIERS_H
