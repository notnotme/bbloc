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
#include "OskLayout.h"

#include <array>
#include <cstring>

#include <utf8.h>


namespace {
    /** @brief One fixed US base entry: the normal and shifted strings of a key slot. */
    struct BaseEntry final {
        const char *normal;  ///< No modifier held.
        const char *shift;   ///< Shift held.
    };

    /** The fixed US base every layout shares: letters, always-plain digits, US punctuation. */
    constexpr std::array<BaseEntry, 256> US_BASE = [] {
        std::array<BaseEntry, 256> t{};
        t[SDL_SCANCODE_A] = { "a", "A" };
        t[SDL_SCANCODE_B] = { "b", "B" };
        t[SDL_SCANCODE_C] = { "c", "C" };
        t[SDL_SCANCODE_D] = { "d", "D" };
        t[SDL_SCANCODE_E] = { "e", "E" };
        t[SDL_SCANCODE_F] = { "f", "F" };
        t[SDL_SCANCODE_G] = { "g", "G" };
        t[SDL_SCANCODE_H] = { "h", "H" };
        t[SDL_SCANCODE_I] = { "i", "I" };
        t[SDL_SCANCODE_J] = { "j", "J" };
        t[SDL_SCANCODE_K] = { "k", "K" };
        t[SDL_SCANCODE_L] = { "l", "L" };
        t[SDL_SCANCODE_M] = { "m", "M" };
        t[SDL_SCANCODE_N] = { "n", "N" };
        t[SDL_SCANCODE_O] = { "o", "O" };
        t[SDL_SCANCODE_P] = { "p", "P" };
        t[SDL_SCANCODE_Q] = { "q", "Q" };
        t[SDL_SCANCODE_R] = { "r", "R" };
        t[SDL_SCANCODE_S] = { "s", "S" };
        t[SDL_SCANCODE_T] = { "t", "T" };
        t[SDL_SCANCODE_U] = { "u", "U" };
        t[SDL_SCANCODE_V] = { "v", "V" };
        t[SDL_SCANCODE_W] = { "w", "W" };
        t[SDL_SCANCODE_X] = { "x", "X" };
        t[SDL_SCANCODE_Y] = { "y", "Y" };
        t[SDL_SCANCODE_Z] = { "z", "Z" };

        t[SDL_SCANCODE_1] = { "1", "!" };
        t[SDL_SCANCODE_2] = { "2", "@" };
        t[SDL_SCANCODE_3] = { "3", "#" };
        t[SDL_SCANCODE_4] = { "4", "$" };
        t[SDL_SCANCODE_5] = { "5", "%" };
        t[SDL_SCANCODE_6] = { "6", "^" };
        t[SDL_SCANCODE_7] = { "7", "&" };
        t[SDL_SCANCODE_8] = { "8", "*" };
        t[SDL_SCANCODE_9] = { "9", "(" };
        t[SDL_SCANCODE_0] = { "0", ")" };

        t[SDL_SCANCODE_SPACE] = { " ", " " };
        t[SDL_SCANCODE_MINUS] = { "-", "_" };
        t[SDL_SCANCODE_EQUALS] = { "=", "+" };
        t[SDL_SCANCODE_LEFTBRACKET] = { "[", "{" };
        t[SDL_SCANCODE_RIGHTBRACKET] = { "]", "}" };
        t[SDL_SCANCODE_APOSTROPHE] = { "'", "\"" };
        t[SDL_SCANCODE_GRAVE] = { "`", "~" };
        t[SDL_SCANCODE_BACKSLASH] = { "\\", "|" };
        t[SDL_SCANCODE_NONUSHASH] = { "\\", "|" };
        t[SDL_SCANCODE_SEMICOLON] = { ";", ":" };
        t[SDL_SCANCODE_COMMA] = { ",", "<" };
        t[SDL_SCANCODE_PERIOD] = { ".", ">" };
        t[SDL_SCANCODE_SLASH] = { "/", "?" };
        t[SDL_SCANCODE_NONUSBACKSLASH] = { "<", ">" };
        return t;
    }();

    // AZERTY: the three-slot letter permutation (a/q, z/w, m next to l — the freed m slot
    // keeps the semicolon pair so nothing disappears).
    constexpr OskLayout::LetterOverride AZERTY_LETTERS[] = {
        { SDL_SCANCODE_Q, "a", "A" },
        { SDL_SCANCODE_A, "q", "Q" },
        { SDL_SCANCODE_W, "z", "Z" },
        { SDL_SCANCODE_Z, "w", "W" },
        { SDL_SCANCODE_SEMICOLON, "m", "M" },
        { SDL_SCANCODE_M, ";", ":" }
    };

    // French accents on mnemonic letters: the vowels carry their most common accent,
    // g(rave) è, x ê, v û, and the œ/æ ligatures land on the free q/j.
    constexpr OskLayout::Accent AZERTY_ACCENTS[] = {
        { "a", "à", "À" },
        { "c", "ç", "Ç" },
        { "e", "é", "É" },
        { "g", "è", "È" },
        { "i", "î", "Î" },
        { "o", "ô", "Ô" },
        { "u", "ù", "Ù" },
        { "x", "ê", "Ê" },
        { "v", "û", "Û" },
        { "q", "œ", "Œ" },
        { "j", "æ", "Æ" }
    };

    // QWERTZ: the y/z swap; umlauts and ß on their base letters.
    constexpr OskLayout::LetterOverride QWERTZ_LETTERS[] = {
        { SDL_SCANCODE_Y, "z", "Z" },
        { SDL_SCANCODE_Z, "y", "Y" }
    };

    constexpr OskLayout::Accent QWERTZ_ACCENTS[] = {
        { "a", "ä", "Ä" },
        { "o", "ö", "Ö" },
        { "u", "ü", "Ü" },
        { "s", "ß", "ẞ" }
    };

    // UK: US letters; the pound joins l.
    constexpr OskLayout::Accent UK_ACCENTS[] = {
        { "l", "£", "£" }
    };

    // Spanish (Spain and Latin America): acute vowels, ñ, and the inverted marks on q/w.
    constexpr OskLayout::Accent SPANISH_ACCENTS[] = {
        { "a", "á", "Á" },
        { "e", "é", "É" },
        { "i", "í", "Í" },
        { "o", "ó", "Ó" },
        { "u", "ú", "Ú" },
        { "n", "ñ", "Ñ" },
        { "q", "¿", "¿" },
        { "w", "¡", "¡" }
    };

    // Italian: grave vowels, plus é on the free q.
    constexpr OskLayout::Accent ITALIAN_ACCENTS[] = {
        { "a", "à", "À" },
        { "e", "è", "È" },
        { "i", "ì", "Ì" },
        { "o", "ò", "Ò" },
        { "u", "ù", "Ù" },
        { "q", "é", "É" }
    };

    // Portuguese: tildes and acutes on their vowels, circumflexes and à on nearby keys.
    constexpr OskLayout::Accent PORTUGUESE_ACCENTS[] = {
        { "a", "ã", "Ã" },
        { "o", "õ", "Õ" },
        { "c", "ç", "Ç" },
        { "e", "é", "É" },
        { "i", "í", "Í" },
        { "u", "ú", "Ú" },
        { "q", "á", "Á" },
        { "w", "â", "Â" },
        { "x", "ê", "Ê" },
        { "v", "ó", "Ó" },
        { "g", "à", "À" }
    };

    // Russian (ЙЦУКЕН): the whole letter zone, extending onto the adjacent punctuation
    // slots for the seven extra letters (х ъ ж э б ю ё), like the physical layout does.
    constexpr OskLayout::LetterOverride RUSSIAN_LETTERS[] = {
        { SDL_SCANCODE_Q, "й", "Й" },
        { SDL_SCANCODE_W, "ц", "Ц" },
        { SDL_SCANCODE_E, "у", "У" },
        { SDL_SCANCODE_R, "к", "К" },
        { SDL_SCANCODE_T, "е", "Е" },
        { SDL_SCANCODE_Y, "н", "Н" },
        { SDL_SCANCODE_U, "г", "Г" },
        { SDL_SCANCODE_I, "ш", "Ш" },
        { SDL_SCANCODE_O, "щ", "Щ" },
        { SDL_SCANCODE_P, "з", "З" },
        { SDL_SCANCODE_A, "ф", "Ф" },
        { SDL_SCANCODE_S, "ы", "Ы" },
        { SDL_SCANCODE_D, "в", "В" },
        { SDL_SCANCODE_F, "а", "А" },
        { SDL_SCANCODE_G, "п", "П" },
        { SDL_SCANCODE_H, "р", "Р" },
        { SDL_SCANCODE_J, "о", "О" },
        { SDL_SCANCODE_K, "л", "Л" },
        { SDL_SCANCODE_L, "д", "Д" },
        { SDL_SCANCODE_Z, "я", "Я" },
        { SDL_SCANCODE_X, "ч", "Ч" },
        { SDL_SCANCODE_C, "с", "С" },
        { SDL_SCANCODE_V, "м", "М" },
        { SDL_SCANCODE_B, "и", "И" },
        { SDL_SCANCODE_N, "т", "Т" },
        { SDL_SCANCODE_M, "ь", "Ь" },
        { SDL_SCANCODE_LEFTBRACKET, "х", "Х" },
        { SDL_SCANCODE_RIGHTBRACKET, "ъ", "Ъ" },
        { SDL_SCANCODE_SEMICOLON, "ж", "Ж" },
        { SDL_SCANCODE_APOSTROPHE, "э", "Э" },
        { SDL_SCANCODE_COMMA, "б", "Б" },
        { SDL_SCANCODE_PERIOD, "ю", "Ю" },
        { SDL_SCANCODE_SLASH, "ё", "Ё" }
    };

    /** Every known layout, by name; qwerty is the plain US base. */
    constexpr std::array<OskLayout::Layout, 9> LAYOUTS = {{
        { "qwerty", {}, {} },
        { "azerty", AZERTY_LETTERS, AZERTY_ACCENTS },
        { "qwertz", QWERTZ_LETTERS, QWERTZ_ACCENTS },
        { "uk", {}, UK_ACCENTS },
        { "spanish", {}, SPANISH_ACCENTS },
        { "spanish_latin", {}, SPANISH_ACCENTS },
        { "italian", {}, ITALIAN_ACCENTS },
        { "portuguese", {}, PORTUGUESE_ACCENTS },
        { "russian", RUSSIAN_LETTERS, {} }
    }};

    /**
     * @brief Finds the accent of a displayed letter, or nullptr when the layout has none.
     *
     * @param layout The active layout.
     * @param letter The displayed lowercase letter. UTF-8.
     * @return The accent entry, or nullptr.
     */
    const OskLayout::Accent *findAccent(const OskLayout::Layout &layout, const char *letter) {
        for (const auto &accent : layout.accents) {
            if (std::strcmp(accent.letter, letter) == 0) {
                return &accent;
            }
        }

        return nullptr;
    }
}

const OskLayout::Layout *OskLayout::findLayout(const std::string_view name) {
    for (const auto &layout : LAYOUTS) {
        if (name == layout.name) {
            return &layout;
        }
    }

    return nullptr;
}

const OskLayout::Layout &OskLayout::defaultLayout() {
    return LAYOUTS[0];
}

const char *OskLayout::resolve(const Layout &layout, const SDL_Scancode scancode, const bool shift, const bool altgr) {
    // A letter-zone override replaces the US base of its slot.
    const char *lower = nullptr;
    const char *upper = nullptr;
    for (const auto &letter : layout.letters) {
        if (letter.scancode == scancode) {
            lower = letter.lower;
            upper = letter.upper;
            break;
        }
    }

    if (lower == nullptr) {
        const auto &base = US_BASE[scancode];
        lower = base.normal;
        upper = base.shift;
    }

    if (lower == nullptr) {
        return nullptr;
    }

    // The AltGr column attaches to the displayed letter; a letter without an accent in
    // this layout falls back to its plain column, so keys never blank out.
    if (altgr) {
        if (const auto *accent = findAccent(layout, lower); accent != nullptr) {
            return shift ? accent->shift_altgr : accent->altgr;
        }
    }

    return shift ? upper : lower;
}

void OskLayout::forEachName(const AutoCompleteCallback &itemCallback) {
    for (const auto &layout : LAYOUTS) {
        itemCallback(utf8::utf8to16(std::string_view(layout.name)));
    }
}
