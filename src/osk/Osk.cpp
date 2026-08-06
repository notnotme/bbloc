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
#include "Osk.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <string>

#include <utf8.h>
#include <utf8/unchecked.h>

#include "../ApplicationWindow.h"
#include "../core/base/PadInput.h"
#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


namespace {
    /** Number of key rows; both pages share the geometry. */
    constexpr int32_t ROW_COUNT = 5;

    /** @brief What a key does when tapped. */
    enum class KeyKind : uint8_t {
        Character,    ///< Injects key events and the layout string of the current state.
        Shifted,      ///< Like Character, but always the shift column, injecting with Shift added.
        Special,      ///< Injects key events only (Esc, Tab, Enter, arrows, ...); fixed label.
        Literal,      ///< Injects a fixed text; the label doubles as the payload (the € key).
        StickyCtrl,   ///< Sticky Control modifier.
        StickyShift,  ///< Sticky Shift modifier.
        StickyAlt,    ///< Sticky left-Alt modifier.
        StickyAltGr,  ///< Sticky AltGr modifier.
        PageToggle,   ///< Flips between the letters and symbols pages.
        Spacer        ///< Empty, unpressable gap.
    };

    /** @brief One key of the grid. */
    struct KeyDef final {
        SDL_Scancode scancode;  ///< Injected scancode; SDL_SCANCODE_UNKNOWN for non-injecting kinds.
        KeyKind kind;           ///< What the key does.
        float width;            ///< Width in row-relative units; rows are normalized to the strip width.
        const char16_t *label;  ///< Fixed label for non-character kinds, nullptr otherwise.
    };

    /** @brief Tells whether holding a key auto-repeats it: every injecting key, like a physical keyboard. */
    constexpr bool isRepeatable(const KeyKind kind) {
        return kind == KeyKind::Character || kind == KeyKind::Shifted || kind == KeyKind::Special || kind == KeyKind::Literal;
    }

    /** @brief One key with its resolved cell rectangle, as visited by forEachKey. */
    struct KeyCell final {
        const KeyDef *p_def;  ///< The key.
        int32_t row;          ///< Row of the key.
        int32_t col;          ///< Column of the key within its row.
        int32_t x;            ///< Cell left edge, window-relative pixels.
        int32_t y;            ///< Cell top edge, window-relative pixels.
        int32_t width;        ///< Cell width in pixels.
        int32_t height;       ///< Cell height in pixels.
    };

    // Page 1 — letters. Esc/digits/Backspace, Tab+top row, Ctrl+home row+Enter,
    // Shift+bottom row+Del+Up, then page toggle/Alt/Space/AltGr/arrows.
    constexpr KeyDef PAGE1_ROW0[] = {
        { SDL_SCANCODE_ESCAPE, KeyKind::Special, 1.5f, u"Esc" },
        { SDL_SCANCODE_1, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_2, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_3, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_4, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_5, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_6, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_7, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_8, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_9, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_0, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_BACKSPACE, KeyKind::Special, 2.0f, u"Bksp" }
    };

    constexpr KeyDef PAGE1_ROW1[] = {
        { SDL_SCANCODE_TAB, KeyKind::Special, 1.5f, u"Tab" },
        { SDL_SCANCODE_Q, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_W, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_E, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_R, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_T, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_Y, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_U, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_I, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_O, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_P, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_LEFTBRACKET, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_RIGHTBRACKET, KeyKind::Character, 1.0f, nullptr }
    };

    constexpr KeyDef PAGE1_ROW2[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyCtrl, 1.8f, u"Ctrl" },
        { SDL_SCANCODE_A, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_S, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_D, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_F, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_G, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_H, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_J, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_K, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_L, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_SEMICOLON, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_APOSTROPHE, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_RETURN, KeyKind::Special, 1.7f, u"Enter" }
    };

    constexpr KeyDef PAGE1_ROW3[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyShift, 2.0f, u"Shift" },
        { SDL_SCANCODE_Z, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_X, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_C, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_V, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_B, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_N, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_M, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_COMMA, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_PERIOD, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_SLASH, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_DELETE, KeyKind::Special, 1.0f, u"Del" },
        { SDL_SCANCODE_UP, KeyKind::Special, 1.0f, u"↑" }
    };

    constexpr KeyDef PAGE1_ROW4[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::PageToggle, 1.5f, u"#+=" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyAlt, 1.5f, u"Alt" },
        { SDL_SCANCODE_SPACE, KeyKind::Character, 6.0f, nullptr },
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyAltGr, 1.5f, u"AltGr" },
        { SDL_SCANCODE_LEFT, KeyKind::Special, 1.0f, u"←" },
        { SDL_SCANCODE_DOWN, KeyKind::Special, 1.0f, u"↓" },
        { SDL_SCANCODE_RIGHT, KeyKind::Special, 1.0f, u"→" }
    };

    // Page 2 — symbols: the shifted characters page 1 does not show, the base keys page 1
    // has no room for (` - = \ <), Home/End/PgUp/PgDn, F3 (the only F key anything binds),
    // and the universal € slot (AltGr+e belongs to the layout accents).
    constexpr KeyDef PAGE2_ROW0[] = {
        { SDL_SCANCODE_ESCAPE, KeyKind::Special, 1.5f, u"Esc" },
        { SDL_SCANCODE_1, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_2, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_3, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_4, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_5, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_6, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_7, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_8, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_9, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_0, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_BACKSPACE, KeyKind::Special, 2.0f, u"Bksp" }
    };

    constexpr KeyDef PAGE2_ROW1[] = {
        { SDL_SCANCODE_TAB, KeyKind::Special, 1.5f, u"Tab" },
        { SDL_SCANCODE_GRAVE, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_MINUS, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_EQUALS, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_BACKSLASH, KeyKind::Character, 1.0f, nullptr },
        { SDL_SCANCODE_GRAVE, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_MINUS, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_EQUALS, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_BACKSLASH, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_LEFTBRACKET, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_RIGHTBRACKET, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_NONUSBACKSLASH, KeyKind::Character, 1.0f, nullptr }
    };

    constexpr KeyDef PAGE2_ROW2[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyCtrl, 1.8f, u"Ctrl" },
        { SDL_SCANCODE_SEMICOLON, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_APOSTROPHE, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_COMMA, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_PERIOD, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_SLASH, KeyKind::Shifted, 1.0f, nullptr },
        { SDL_SCANCODE_HOME, KeyKind::Special, 1.3f, u"Home" },
        { SDL_SCANCODE_END, KeyKind::Special, 1.3f, u"End" },
        { SDL_SCANCODE_PAGEUP, KeyKind::Special, 1.3f, u"PgUp" },
        { SDL_SCANCODE_PAGEDOWN, KeyKind::Special, 1.3f, u"PgDn" },
        { SDL_SCANCODE_RETURN, KeyKind::Special, 1.7f, u"Enter" }
    };

    constexpr KeyDef PAGE2_ROW3[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyShift, 2.0f, u"Shift" },
        { SDL_SCANCODE_F3, KeyKind::Special, 1.0f, u"F3" },
        // Symbols no key combination reaches: the layout accents cover the letters, and
        // AltGr+e is spent on €. They fill the row to the 13 keys the letters page has.
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"€" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"£" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"¥" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"°" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"§" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"µ" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"×" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"÷" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::Literal, 1.0f, u"…" },
        { SDL_SCANCODE_DELETE, KeyKind::Special, 1.0f, u"Del" },
        { SDL_SCANCODE_UP, KeyKind::Special, 1.0f, u"↑" }
    };

    constexpr KeyDef PAGE2_ROW4[] = {
        { SDL_SCANCODE_UNKNOWN, KeyKind::PageToggle, 1.5f, u"abc" },
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyAlt, 1.5f, u"Alt" },
        { SDL_SCANCODE_SPACE, KeyKind::Character, 6.0f, nullptr },
        { SDL_SCANCODE_UNKNOWN, KeyKind::StickyAltGr, 1.5f, u"AltGr" },
        { SDL_SCANCODE_LEFT, KeyKind::Special, 1.0f, u"←" },
        { SDL_SCANCODE_DOWN, KeyKind::Special, 1.0f, u"↓" },
        { SDL_SCANCODE_RIGHT, KeyKind::Special, 1.0f, u"→" }
    };

    constexpr std::span<const KeyDef> PAGE1_ROWS[ROW_COUNT] = { PAGE1_ROW0, PAGE1_ROW1, PAGE1_ROW2, PAGE1_ROW3, PAGE1_ROW4 };
    constexpr std::span<const KeyDef> PAGE2_ROWS[ROW_COUNT] = { PAGE2_ROW0, PAGE2_ROW1, PAGE2_ROW2, PAGE2_ROW3, PAGE2_ROW4 };

    /** @brief Gets the rows of a page. */
    std::span<const std::span<const KeyDef>> pageRows(const int32_t page) {
        return page == 0 ? PAGE1_ROWS : PAGE2_ROWS;
    }

    /** @brief Gets a key by grid position, or nullptr when out of range. */
    const KeyDef *keyAt(const int32_t page, const int32_t row, const int32_t col) {
        const auto rows = pageRows(page);
        if (row < 0 || row >= static_cast<int32_t>(rows.size())) {
            return nullptr;
        }

        const auto keys = rows[row];
        if (col < 0 || col >= static_cast<int32_t>(keys.size())) {
            return nullptr;
        }

        return &keys[col];
    }

    /** @brief Sums the unit widths of a row. */
    float rowUnits(const std::span<const KeyDef> keys) {
        auto units = 0.0f;
        for (const auto &key : keys) {
            units += key.width;
        }

        return units;
    }

    /**
     * @brief Visits every key of the visible page with its cell rectangle.
     *
     * Each row is normalized: unit widths are scaled so the row spans the strip width.
     *
     * @param viewState The OSK view state, holding the page and the strip rectangle.
     * @param callback Invoked with a KeyCell per key, spacers included.
     */
    template <typename TCallback>
    void forEachKey(const OskState &viewState, TCallback &&callback) {
        const auto rows = pageRows(viewState.getPage());
        const auto position_x = viewState.getPositionX();
        const auto position_y = viewState.getPositionY();
        const auto width = static_cast<float>(viewState.getWidth());
        const auto row_height = viewState.getHeight() / ROW_COUNT;

        for (auto row = 0; row < ROW_COUNT; ++row) {
            const auto keys = rows[row];
            const auto total_units = rowUnits(keys);
            auto accumulated_units = 0.0f;
            for (auto col = 0; col < static_cast<int32_t>(keys.size()); ++col) {
                const auto x_start = position_x + static_cast<int32_t>(std::lround(accumulated_units / total_units * width));
                accumulated_units += keys[col].width;
                const auto x_end = position_x + static_cast<int32_t>(std::lround(accumulated_units / total_units * width));
                callback(KeyCell {
                    &keys[col], row, col,
                    x_start, position_y + row * row_height,
                    x_end - x_start, row_height
                });
            }
        }
    }

    /** @brief Maps a sticky key kind to its modifier, or nothing for other kinds. */
    std::optional<OskState::StickyModifier> stickyModifierFor(const KeyKind kind) {
        switch (kind) {
            case KeyKind::StickyCtrl:
                return OskState::StickyModifier::Ctrl;
            case KeyKind::StickyShift:
                return OskState::StickyModifier::Shift;
            case KeyKind::StickyAlt:
                return OskState::StickyModifier::Alt;
            case KeyKind::StickyAltGr:
                return OskState::StickyModifier::AltGr;
            default:
                return std::nullopt;
        }
    }

    /**
     * @brief Advances a sticky modifier one step in the Idle -> Latched -> Held -> Idle cycle.
     *
     * Every press path uses it, so holding a modifier is reachable by tapping alone: the pad
     * has no press duration to measure, and a long-press on a touch screen is easy to miss.
     *
     * @param state The current sticky state of the modifier.
     * @return The state the press moves it to.
     */
    OskState::StickyState nextStickyState(const OskState::StickyState state) {
        switch (state) {
            case OskState::StickyState::Idle:
                return OskState::StickyState::Latched;
            case OskState::StickyState::Latched:
                return OskState::StickyState::Held;
            default:
                return OskState::StickyState::Idle;
        }
    }

    /**
     * @brief Resolves the layout string of a character key: its label and TEXTINPUT payload.
     *
     * Delegates to the hybrid layout: the fixed US base, the letter permutation, and the
     * AltGr accent column looked up by displayed letter (no caps lock on the OSK).
     *
     * @param viewState The OSK view state, holding the layout.
     * @param key The Character or Shifted key to resolve.
     * @param modifiers The sticky KMOD mask to resolve under (live for labels, captured for repeats).
     * @return The UTF-8 string, or nullptr when the base defines nothing for that slot.
     */
    const char *resolveText(const OskState &viewState, const KeyDef &key, const uint16_t modifiers) {
        const auto shift = key.kind == KeyKind::Shifted || (modifiers & KMOD_LSHIFT) != 0;
        const auto altgr = (modifiers & KMOD_RALT) != 0;
        return OskLayout::resolve(viewState.getLayout(), key.scancode, shift, altgr);
    }

    /**
     * @brief Resolves the keycode a key injects.
     *
     * The keycode follows the displayed letter, not the slot: the azerty a key (sitting on
     * the qwerty q slot) injects SDLK_a, so chords always match the letter the user sees.
     * Literal keys take their label codepoint; other keys use the SDL default keymap.
     *
     * @param viewState The OSK view state, holding the layout.
     * @param key The key to resolve.
     * @return The SDL keycode to put in the synthesized event.
     */
    SDL_Keycode resolveKeycode(const OskState &viewState, const KeyDef &key) {
        if (key.kind == KeyKind::Literal) {
            // A single BMP character by construction (the € key).
            return static_cast<SDL_Keycode>(key.label[0]);
        }

        if (key.kind == KeyKind::Character || key.kind == KeyKind::Shifted) {
            if (const auto *lower = OskLayout::resolve(viewState.getLayout(), key.scancode, false, false); lower != nullptr) {
                // First unicode codepoint of the UTF-8 string; keycodes are unicode codepoints.
                // The layout strings are trusted static data, the unchecked reader is fine.
                return static_cast<SDL_Keycode>(utf8::unchecked::peek_next(lower));
            }
        }

        return SDL_GetKeyFromScancode(key.scancode);
    }

    /**
     * @brief Pushes the synthesized events of one key tap: KEYDOWN, optional TEXTINPUT, KEYUP.
     *
     * The sticky-modifier mask rides in keysym.mod, so chords work without touching the real
     * SDL modifier state. No TEXTINPUT is synthesized while Ctrl or Alt is latched.
     *
     * @param viewState The OSK view state, holding the layout.
     * @param key The tapped key; must be a Character, Shifted, Special, or Literal key.
     * @param stickyModifiers The sticky KMOD mask to inject under; captured at press time, so
     *                        a hold keeps re-emitting the same events after the latches release.
     */
    void injectTap(const OskState &viewState, const KeyDef &key, const uint16_t stickyModifiers) {
        const auto modifiers = static_cast<uint16_t>(stickyModifiers | (key.kind == KeyKind::Shifted ? KMOD_LSHIFT : 0));
        const auto keycode = resolveKeycode(viewState, key);
        const auto timestamp = SDL_GetTicks();

        // The scancode follows the displayed letter too, where the host keymap has one
        // (an accented or cyrillic keycode does not; the slot scancode then stands in).
        auto scancode = key.scancode;
        if (key.kind != KeyKind::Special) {
            if (const auto derived = SDL_GetScancodeFromKey(keycode); derived != SDL_SCANCODE_UNKNOWN) {
                scancode = derived;
            }
        }

        auto key_event = SDL_Event{};
        key_event.key.type = SDL_KEYDOWN;
        key_event.key.timestamp = timestamp;
        key_event.key.state = SDL_PRESSED;
        key_event.key.repeat = 0;
        key_event.key.keysym.scancode = scancode;
        key_event.key.keysym.sym = keycode;
        key_event.key.keysym.mod = modifiers;
        SDL_PushEvent(&key_event);

        // Printable keys compose text, unless a control chord is latched (like real keyboards).
        // The text rides a user event instead of SDL_TEXTINPUT (see Osk::textEventType).
        const auto is_character = key.kind == KeyKind::Character || key.kind == KeyKind::Shifted || key.kind == KeyKind::Literal;
        if (is_character && (modifiers & (KMOD_LCTRL | KMOD_LALT)) == 0) {
            auto text = std::string{};
            if (key.kind == KeyKind::Literal) {
                // The label doubles as the payload (the € key).
                text = utf8::utf16to8(std::u16string_view(key.label));
            } else if (const auto *resolved = resolveText(viewState, key, stickyModifiers); resolved != nullptr) {
                text = resolved;
            }

            if (!text.empty()) {
                auto text_event = SDL_Event{};
                text_event.user.type = Osk::textEventType();
                text_event.user.timestamp = timestamp;
                text_event.user.data1 = SDL_strdup(text.c_str());
                SDL_PushEvent(&text_event);
            }
        }

        key_event.key.type = SDL_KEYUP;
        key_event.key.state = SDL_RELEASED;
        SDL_PushEvent(&key_event);
    }

    /** @brief Packs a grid position into an InputRepeater code. */
    constexpr int32_t encodeKey(const int32_t page, const int32_t row, const int32_t col) {
        return (page << 16) | (row << 8) | col;
    }

    /** @brief Gets the normalized horizontal center (0..1) of a key within its row. */
    float unitCenter(const std::span<const KeyDef> keys, const int32_t col) {
        auto accumulated_units = 0.0f;
        for (auto index = 0; index < col; ++index) {
            accumulated_units += keys[index].width;
        }

        return (accumulated_units + keys[col].width / 2.0f) / rowUnits(keys);
    }

    /** @brief Finds the non-spacer key of a row whose center is nearest to a normalized position. */
    int32_t nearestCol(const std::span<const KeyDef> keys, const float centerFraction) {
        auto best_col = 0;
        auto best_distance = 2.0f;
        for (auto col = 0; col < static_cast<int32_t>(keys.size()); ++col) {
            if (keys[col].kind == KeyKind::Spacer) {
                continue;
            }

            const auto distance = std::abs(unitCenter(keys, col) - centerFraction);
            if (distance < best_distance) {
                best_distance = distance;
                best_col = col;
            }
        }

        return best_col;
    }

    /** @brief Clamps the pad key cursor into the visible page, off any spacer. */
    void clampCursor(OskState &viewState) {
        const auto rows = pageRows(viewState.getPage());
        const auto row = std::clamp(viewState.getCursorRow(), 0, ROW_COUNT - 1);
        auto col = std::clamp(viewState.getCursorCol(), 0, static_cast<int32_t>(rows[row].size()) - 1);
        if (rows[row][col].kind == KeyKind::Spacer) {
            col = nearestCol(rows[row], unitCenter(rows[row], col));
        }

        viewState.setCursor(row, col);
    }
}

Osk::Osk(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram)
    : View(commandController, theme, quadProgram) {}

uint32_t Osk::textEventType() {
    // Registered once on first use; a single registration never exhausts the range.
    static const auto type = SDL_RegisterEvents(1);
    return type;
}

void Osk::render(CursorContext &context, OskState &viewState, QuadBuffer &quadBuffer, float dt) {
    (void) dt;
    if (!viewState.isVisible()) {
        return;
    }

    const auto batch_start = quadBuffer.beginBatch(ApplicationWindow::OSK_DEFAULT_QUAD_COUNT);
    drawKeys(quadBuffer, context, viewState);
    const auto batch_count = quadBuffer.endBatch();

    // Get the view geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Set the scissor area and draw the buffer
    glScissor(position_x, m_window_height - position_y - height, width, height);
    m_quad_program.draw(batch_start, batch_count);
}

void Osk::drawKeys(QuadBuffer &quadBuffer, const CursorContext &context, const OskState &viewState) {
    // The strip background, under everything
    const auto &background_color = m_theme.getColor(ColorId::OskBackground);
    drawQuad(quadBuffer, viewState.getPositionX(), viewState.getPositionY(), viewState.getWidth(), viewState.getHeight(), background_color);

    // Keep some variables that are frequently needed
    const auto &key_color = m_theme.getColor(ColorId::OskKeyBackground);
    const auto &pressed_color = m_theme.getColor(ColorId::OskKeyPressed);
    const auto &text_color = m_theme.getColor(ColorId::OskKeyText);
    const auto &cursor_color = m_theme.getColor(ColorId::OskKeyCursor);
    const auto &dot_color = m_theme.getColor(ColorId::CursorIndicator);
    const auto gap = m_theme.getDimension(DimensionId::OskKeyGap);
    const auto dot_side = m_theme.getDimension(DimensionId::IndicatorWidth) * 2;
    const auto line_height = m_theme.getLineHeight();
    const auto font_advance = m_theme.getFontAdvance();
    const auto font_descender = m_theme.getFontDescender();

    // The pad key cursor only shows while the OSK has the pad focus
    const auto cursor_visible = context.focus_target == FocusTarget::Osk;

    forEachKey(viewState, [&](const KeyCell &cell) {
        if (cell.p_def->kind == KeyKind::Spacer) {
            return;
        }

        // The key face, inset by the whole gap on every side so the strip background shows
        // through evenly — a half-gap inset rounds to zero on the left and top at gap 1,
        // leaving the contour only on the right and bottom. The held key lights up.
        const auto is_pressed = cell.row == viewState.getPressedRow() && cell.col == viewState.getPressedCol();
        const auto key_x = cell.x + gap;
        const auto key_y = cell.y + gap;
        const auto key_width = std::max(cell.width - gap * 2, 1);
        const auto key_height = std::max(cell.height - gap * 2, 1);
        drawQuad(quadBuffer, key_x, key_y, key_width, key_height, is_pressed ? pressed_color : key_color);

        // The pad key cursor tints the face it sits on, rather than ringing the cell behind
        // it: the ring a face painted over leaves is exactly the gap, so it vanishes at gap
        // 0. The cursor color is translucent, so the face — the pressed color included —
        // stays readable through it, and the label drawn next lands on top of the tint.
        if (cursor_visible && cell.row == viewState.getCursorRow() && cell.col == viewState.getCursorCol()) {
            drawQuad(quadBuffer, key_x, key_y, key_width, key_height, cursor_color);
        }

        // Resolve the label: fixed for special keys, from the layout (under the live sticky
        // mask, so Shift/AltGr flip the labels) for character keys
        auto label = std::u16string{};
        if (cell.p_def->label != nullptr) {
            label = cell.p_def->label;
        } else if (const auto *text = resolveText(viewState, *cell.p_def, viewState.effectiveModifierMask()); text != nullptr) {
            label = utf8::utf8to16(std::string_view(text));
        }

        // Center the label on the key
        const auto label_width = static_cast<int32_t>(m_theme.measure(label));
        auto pen_x = cell.x + (cell.width - label_width) / 2;
        const auto pen_y = cell.y + (cell.height - line_height) / 2 + line_height + font_descender;
        for (const auto c : label) {
            const auto &character = m_theme.getCharacter(c);
            drawCharacter(quadBuffer, pen_x, pen_y, character, text_color);
            pen_x += font_advance;
        }

        // Sticky state dot: single for latched, doubled for held
        if (const auto modifier = stickyModifierFor(cell.p_def->kind); modifier.has_value()) {
            const auto sticky = viewState.getSticky(*modifier);
            if (sticky != OskState::StickyState::Idle) {
                const auto dot_width = sticky == OskState::StickyState::Held ? dot_side * 2 : dot_side;
                drawQuad(quadBuffer, key_x + key_width - dot_width - gap, key_y + gap, dot_width, dot_side, dot_color);
            }
        }
    });
}

bool Osk::onKeyDown(CursorContext &context, OskState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    (void) context;
    (void) viewState;
    (void) keyCode;
    (void) keyModifier;
    return false;
}

void Osk::onTextInput(CursorContext &context, OskState &viewState, const char *text) const {
    (void) context;
    (void) viewState;
    (void) text;
    // No-op
}

void Osk::onMouseDown(CursorContext &context, OskState &viewState, const int32_t x, const int32_t y) {
    if (!viewState.isVisible()) {
        return;
    }

    forEachKey(viewState, [&](const KeyCell &cell) {
        if (cell.p_def->kind == KeyKind::Spacer
            || x < cell.x || x >= cell.x + cell.width
            || y < cell.y || y >= cell.y + cell.height) {
            return;
        }

        // Remember the press so the release can match it (sticky settle, repeat disarm)
        viewState.setPressed(cell.row, cell.col, SDL_GetTicks64());
        context.wants_redraw = true;

        if (stickyModifierFor(cell.p_def->kind).has_value()) {
            // Sticky keys settle on release: a tap latches, a long-press holds
            return;
        }

        if (cell.p_def->kind == KeyKind::PageToggle) {
            viewState.setPage(1 - viewState.getPage());
            viewState.getRepeater().disarm();
            return;
        }

        // Regular key: inject now, consume the one-shot latches, and arm the hold repeat
        // with the mask captured at press, so the repeats keep emitting the same events
        const auto sticky_modifiers = viewState.effectiveModifierMask();
        injectTap(viewState, *cell.p_def, sticky_modifiers);
        viewState.releaseLatched();
        viewState.getRepeater().disarm();
        if (isRepeatable(cell.p_def->kind)) {
            viewState.getRepeater().arm(encodeKey(viewState.getPage(), cell.row, cell.col), sticky_modifiers);
            // Remember what this tap typed into: the repeats must not follow the focus elsewhere
            viewState.setRepeatTarget(context.effectiveFocus());
        }
    });
}

void Osk::onMouseUp(CursorContext &context, OskState &viewState, const int32_t x, const int32_t y) {
    (void) x;
    (void) y;
    if (viewState.getPressedRow() < 0) {
        return;
    }

    // Sticky keys settle now: long-press holds, a tap toggles latched/idle
    const auto *key = keyAt(viewState.getPage(), viewState.getPressedRow(), viewState.getPressedCol());
    if (key != nullptr) {
        if (const auto modifier = stickyModifierFor(key->kind); modifier.has_value()) {
            // A long press jumps straight to Held; a tap steps through the cycle, so the
            // hold is also reachable by tapping twice when the press duration is missed.
            const auto held_long = SDL_GetTicks64() - viewState.getPressTime() >= LONG_PRESS_MS;
            viewState.setSticky(*modifier, held_long ? OskState::StickyState::Held : nextStickyState(viewState.getSticky(*modifier)));
        }
    }

    // The release always ends the hold repeat and clears the pressed highlight
    viewState.getRepeater().disarm();
    viewState.clearPressed();
    context.wants_redraw = true;
}

bool Osk::onPadInput(CursorContext &context, OskState &viewState, const SDL_Keycode padKeycode) const {
    clampCursor(viewState);
    const auto rows = pageRows(viewState.getPage());
    const auto row = viewState.getCursorRow();
    const auto col = viewState.getCursorCol();

    if (padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        || padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
        // Step within the row, skipping spacers
        const auto step = padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? -1 : 1;
        auto next_col = col + step;
        while (next_col >= 0 && next_col < static_cast<int32_t>(rows[row].size()) && rows[row][next_col].kind == KeyKind::Spacer) {
            next_col += step;
        }

        if (next_col >= 0 && next_col < static_cast<int32_t>(rows[row].size())) {
            viewState.setCursor(row, next_col);
        }
        context.wants_redraw = true;
        return true;
    }

    if (padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_UP)
        || padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
        // Step to the neighbor row, landing on the key nearest to the current center
        const auto step = padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_UP) ? -1 : 1;
        const auto next_row = std::clamp(row + step, 0, ROW_COUNT - 1);
        if (next_row != row) {
            viewState.setCursor(next_row, nearestCol(rows[next_row], unitCenter(rows[row], col)));
        }
        context.wants_redraw = true;
        return true;
    }

    if (padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_A)) {
        // Press the key under the cursor; sticky keys toggle (no long-press on a tap button)
        const auto &key = rows[row][col];
        if (const auto modifier = stickyModifierFor(key.kind); modifier.has_value()) {
            // A tap button has no press duration: stepping the cycle is the only way the
            // pad can reach the Held state, so A cycles Idle -> Latched -> Held -> Idle.
            viewState.setSticky(*modifier, nextStickyState(viewState.getSticky(*modifier)));
        } else if (key.kind == KeyKind::PageToggle) {
            viewState.setPage(1 - viewState.getPage());
            viewState.getRepeater().disarm();
        } else if (key.kind != KeyKind::Spacer) {
            injectTap(viewState, key, viewState.effectiveModifierMask());
            viewState.releaseLatched();
        }
        context.wants_redraw = true;
        return true;
    }

    if (padKeycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_B)) {
        if (context.osk_return_focus == FocusTarget::Prompt) {
            // B over an active prompt cancels it outright: handing the pad back first
            // would cost a second B for what reads as one "get me out of here".
            // The OSK keeps the pad, now over the editor, so typing can continue.
            context.osk_return_focus = FocusTarget::Editor;
            context.command_runner.runCommand(u"prompt cancel", false);
            context.focus_target = FocusTarget::Osk;
            context.wants_redraw = true;
            return true;
        }

        // Hand the pad back to where it was taken from; the OSK stays visible for touch
        context.focus_target = context.osk_return_focus;
        context.wants_redraw = true;
        return true;
    }

    return false;
}

void Osk::tickRepeat(CursorContext &context, OskState &viewState) const {
    if (!viewState.getRepeater().isDue()) {
        return;
    }

    // A repeat only keeps firing while it still lands where the press did. The tap may have run
    // a command — Enter over the prompt opens a file — which closes the prompt and makes another
    // buffer active. The release ending the hold is polled only after that command returned, so
    // on a slow open the deadline passes first and the repeat would type into the file just
    // opened, leaving a blank line at its top.
    if (!viewState.isRepeatTarget(context.effectiveFocus())) {
        viewState.getRepeater().disarm();
        return;
    }

    // Decode the held key and re-tap it under the sticky mask captured at press time, so a
    // hold keeps emitting the same events even after the one-shot latches released
    const auto code = viewState.getRepeater().getCode();
    const auto *key = keyAt((code >> 16) & 0xFF, (code >> 8) & 0xFF, code & 0xFF);
    if (key == nullptr || !isRepeatable(key->kind)) {
        viewState.getRepeater().disarm();
        return;
    }

    injectTap(viewState, *key, viewState.getRepeater().getModifiers());
    viewState.getRepeater().rearm();
}
