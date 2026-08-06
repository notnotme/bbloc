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
#ifndef OSK_LAYOUT_H
#define OSK_LAYOUT_H

#include <span>
#include <string_view>

#include <SDL_scancode.h>

#include "../core/base/AutoCompleteCallback.h"


/**
 * @brief Static-only registry of the on-screen keyboard hybrid layouts.
 *
 * The OSK does not imitate physical national keyboards. Every layout shares one fixed US
 * base: the digit row is always plain digits (Shift gives the US symbols), and every
 * punctuation key keeps its US semantics, so brackets and friends live in one predictable
 * place everywhere. A layout only contributes:
 *
 * - a **letter permutation** (azerty swaps a/q, z/w and moves m; qwertz swaps y/z; russian
 *   replaces the whole letter zone and extends onto the adjacent punctuation slots for its
 *   extra letters), and
 * - an **accent map**: the AltGr column on mnemonic letters (azerty AltGr+e → é) with the
 *   Shift+AltGr column giving the proper uppercase accents (É À Ç ...).
 *
 * Strings are UTF-8 on purpose — SDL_TEXTINPUT is a UTF-8 boundary; labels are converted
 * for rendering. The injected keycode follows the displayed letter, not the slot.
 */
class OskLayout final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    OskLayout() = delete;

    /** @brief One letter-zone slot a layout overrides (permuted or national letter). */
    struct LetterOverride final {
        SDL_Scancode scancode;  ///< The key slot being overridden.
        const char *lower;      ///< Displayed/emitted letter, lowercase. UTF-8.
        const char *upper;      ///< Displayed/emitted letter under Shift. UTF-8.
    };

    /** @brief One AltGr accent, attached to a displayed letter (not to a slot). */
    struct Accent final {
        const char *letter;       ///< The displayed lowercase letter carrying the accent. UTF-8.
        const char *altgr;        ///< AltGr column: the accented character. UTF-8.
        const char *shift_altgr;  ///< Shift+AltGr column: the uppercase accent. UTF-8.
    };

    /** @brief One hybrid layout: a name, a letter permutation, and an accent map. */
    struct Layout final {
        const char *name;                          ///< Layout name, plain ASCII.
        std::span<const LetterOverride> letters;   ///< Letter-zone overrides; empty keeps US letters.
        std::span<const Accent> accents;           ///< AltGr accents; empty for none.
    };

    /**
     * @brief Finds a layout by name.
     *
     * Known names: qwerty, azerty, qwertz, uk, spanish, spanish_latin, italian, portuguese,
     * russian.
     *
     * @param name The layout name. UTF-8 (layout names are plain ASCII).
     * @return The layout, or nullptr when the name is unknown.
     */
    [[nodiscard]] static const Layout *findLayout(std::string_view name);

    /**
     * @brief Gets the default layout (qwerty: the plain US base, no overrides).
     */
    [[nodiscard]] static const Layout &defaultLayout();

    /**
     * @brief Resolves the UTF-8 string a key slot displays and emits under a modifier state.
     *
     * Letter overrides apply first; anything else falls back to the fixed US base. The
     * AltGr column is looked up by the displayed letter, and falls back to the plain
     * letter when the layout defines no accent for it.
     *
     * @param layout The active layout.
     * @param scancode The key slot.
     * @param shift True when Shift applies (sticky latch or a shifted page-2 key).
     * @param altgr True when AltGr applies.
     * @return The UTF-8 string, or nullptr when the base defines nothing for the slot.
     */
    [[nodiscard]] static const char *resolve(const Layout &layout, SDL_Scancode scancode, bool shift, bool altgr);

    /**
     * @brief Enumerates every known layout name, for auto-completion.
     *
     * @param itemCallback A callback invoked with each name.
     */
    static void forEachName(const AutoCompleteCallback &itemCallback);
};


#endif //OSK_LAYOUT_H
