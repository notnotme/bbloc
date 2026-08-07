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
#ifndef OSK_STATE_H
#define OSK_STATE_H

#include <array>
#include <cstdint>

#include <SDL_keycode.h>

#include "../core/FocusTarget.h"
#include "../core/ViewState.h"
#include "../input/InputRepeater.h"
#include "OskLayout.h"


/**
 * @brief Stores the layout, visibility, and interaction state of the on-screen keyboard view.
 *
 * Tracks the visible page, the active layout table, the sticky modifier states, the
 * pad-navigation key cursor, the currently pressed key and the source holding it, the hold
 * auto-repeat, and whether the on-screen keyboard currently owns the game pad. The pad grab
 * lives here, and not in a CursorContext, because the on-screen keyboard is one object shared
 * by every buffer.
 */
class OskState final : public ViewState {
public:
    /** @brief The sticky modifiers the on-screen keyboard offers. */
    enum class StickyModifier : uint8_t {
        Ctrl,   ///< Control, on the Caps position.
        Shift,  ///< Shift, also flips the displayed labels.
        Alt,    ///< Left Alt.
        AltGr   ///< Right Alt, selects the altgr layout column.
    };

    /** @brief State of one sticky modifier. */
    enum class StickyState : uint8_t {
        Idle,     ///< Not active.
        Latched,  ///< Active for the next key only (tap).
        Held      ///< Active until tapped again (long-press).
    };

    /**
     * @brief The input source holding the currently pressed key.
     *
     * There is one press slot, so the first holder keeps the key: while a pointer holds one, a
     * pad A press is ignored, and the other way around. The source records that holder, so the
     * sticky settle only runs for the release matching the press it started from.
     */
    enum class PressSource : uint8_t {
        None,     ///< No key is pressed.
        Pointer,  ///< A mouse button or a finger holds it.
        Pad       ///< The game pad A button holds it.
    };

    /** @brief Number of StickyModifier values; must track the last enumerator. */
    static constexpr std::size_t STICKY_MODIFIER_COUNT = static_cast<std::size_t>(StickyModifier::AltGr) + 1;

private:
    /** True while the on-screen keyboard is shown (and laid out). */
    bool m_visible;

    /** Visible key page: 0 = letters, 1 = symbols. */
    int32_t m_page;

    /** Active hybrid layout; never null. */
    const OskLayout::Layout *p_layout;

    /** State of each sticky modifier, indexed by StickyModifier. */
    std::array<StickyState, STICKY_MODIFIER_COUNT> m_sticky;

    /** KMOD mask held down on the hardware right now, fed by the input classes. */
    uint16_t m_live_modifiers;

    /** Row of the pad-navigation key cursor. */
    int32_t m_cursor_row;

    /** Column of the pad-navigation key cursor within its row. */
    int32_t m_cursor_col;

    /** Row of the key currently pressed, -1 when none. */
    int32_t m_pressed_row;

    /** Column of the key currently pressed. */
    int32_t m_pressed_col;

    /** SDL_GetTicks64 time of the current press, to tell taps from long-presses. */
    uint64_t m_press_time;

    /** Input source holding the current press, PressSource::None when none. */
    PressSource m_press_source;

    /** Auto-repeat state of the held key (arrows, Backspace, Delete). */
    InputRepeater m_repeater;

    /** Keyboard focus the press that armed the repeat was delivered to. */
    FocusTarget m_repeat_focus;

    /** True while the on-screen keyboard owns the game pad (its key cursor is shown and driven). */
    bool m_pad_focus;

public:
    /** @brief Deleted copy constructor. */
    OskState(const OskState &) = delete;

    /** @brief Deleted copy assignment operator. */
    OskState &operator=(const OskState &) = delete;

    /** @brief Constructs a hidden OskState on the platform keyboard layout. */
    explicit OskState();

    /** @brief Tells whether the on-screen keyboard is shown. */
    [[nodiscard]] bool isVisible() const;

    /** @brief Gets the visible key page (0 = letters, 1 = symbols). */
    [[nodiscard]] int32_t getPage() const;

    /** @brief Gets the active hybrid layout; never null. */
    [[nodiscard]] const OskLayout::Layout &getLayout() const;

    /** @brief Gets the state of a sticky modifier. */
    [[nodiscard]] StickyState getSticky(StickyModifier modifier) const;

    /**
     * @brief Builds the modifier mask the sticky states inject into synthesized key events.
     *
     * @return A KMOD mask: LCTRL/LSHIFT/LALT/RALT bits for the non-idle sticky modifiers.
     */
    [[nodiscard]] uint16_t stickyModifierMask() const;

    /**
     * @brief Builds the modifier mask a key tap actually injects and labels resolve under.
     *
     * The sticky latches plus whatever is held on the hardware, so a pad shoulder modifies
     * an on-screen key the way holding Shift modifies a physical one.
     *
     * @return The sticky mask ORed with the live mask.
     */
    [[nodiscard]] uint16_t effectiveModifierMask() const;

    /**
     * @brief Sets the modifier mask held on the hardware, in KMOD bits.
     *
     * Called by the input classes when a held modifier they map to the on-screen keyboard
     * changes (the controller shoulders); it is not the sticky state and is never latched.
     *
     * @param modifiers The KMOD mask currently held, 0 for none.
     */
    void setLiveModifiers(uint16_t modifiers);

    /** @brief Gets the row of the pad-navigation key cursor. */
    [[nodiscard]] int32_t getCursorRow() const;

    /** @brief Gets the column of the pad-navigation key cursor. */
    [[nodiscard]] int32_t getCursorCol() const;

    /** @brief Gets the row of the pressed key, -1 when none. */
    [[nodiscard]] int32_t getPressedRow() const;

    /** @brief Gets the column of the pressed key. */
    [[nodiscard]] int32_t getPressedCol() const;

    /** @brief Gets the SDL_GetTicks64 time of the current press. */
    [[nodiscard]] uint64_t getPressTime() const;

    /** @brief Gets the input source holding the current press, PressSource::None when none. */
    [[nodiscard]] PressSource getPressSource() const;

    /** @brief Gets the hold auto-repeat state. */
    [[nodiscard]] InputRepeater &getRepeater();

    /** @brief Gets the hold auto-repeat state, read-only. */
    [[nodiscard]] const InputRepeater &getRepeater() const;

    /**
     * @brief Records what the armed repeat types into, captured when the key was pressed.
     *
     * @param focus The effective focus the press was delivered to.
     */
    void setRepeatTarget(FocusTarget focus);

    /**
     * @brief Tells whether a repeat would still land where its press did.
     *
     * @param focus The keyboard focus a repeat would be delivered to.
     * @return true when it matches the one captured at press time.
     */
    [[nodiscard]] bool isRepeatTarget(FocusTarget focus) const;

    /** @brief Tells whether the on-screen keyboard owns the game pad. */
    [[nodiscard]] bool hasPadFocus() const;

    /**
     * @brief Grabs or releases the game pad.
     *
     * Taken lazily, on the first pad input routed to a visible on-screen keyboard, so mouse
     * and touch users never see the key cursor; released by pad B and by hiding the keyboard.
     *
     * @param owned True when the on-screen keyboard owns the pad.
     */
    void setPadFocus(bool owned);

    /**
     * @brief Shows or hides the on-screen keyboard.
     *
     * @param visible The new visibility.
     */
    void setVisible(bool visible);

    /**
     * @brief Sets the visible key page.
     *
     * @param page The new page (0 = letters, 1 = symbols).
     */
    void setPage(int32_t page);

    /**
     * @brief Sets the active hybrid layout.
     *
     * @param layout The new layout.
     */
    void setLayout(const OskLayout::Layout &layout);

    /**
     * @brief Sets the state of a sticky modifier.
     *
     * @param modifier The sticky modifier to change.
     * @param state Its new state.
     */
    void setSticky(StickyModifier modifier, StickyState state);

    /** @brief Releases the one-shot latches: every Latched modifier returns to Idle, Held ones persist. */
    void releaseLatched();

    /**
     * @brief Places the pad-navigation key cursor.
     *
     * @param row The new row.
     * @param col The new column within the row.
     */
    void setCursor(int32_t row, int32_t col);

    /**
     * @brief Records the pressed key and what holds it, to match them on release.
     *
     * The source matters because both a pointer and the pad A button can start a press: the
     * release only ends it when it comes from the source that started it, and the caller keeps
     * the other source out of the single slot while that one holds it.
     *
     * @param row Row of the pressed key.
     * @param col Column of the pressed key.
     * @param pressTime SDL_GetTicks64 time of the press.
     * @param source The input source holding the key down.
     */
    void setPressed(int32_t row, int32_t col, uint64_t pressTime, PressSource source);

    /** @brief Forgets the pressed key and its source. */
    void clearPressed();

    /**
     * @brief Resets the transient interaction state: stickies, page, press, repeat, and the
     *        pad focus. Used on hide.
     */
    void resetInteraction();
};


#endif //OSK_STATE_H
