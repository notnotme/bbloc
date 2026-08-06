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
#ifndef INPUT_REPEATER_H
#define INPUT_REPEATER_H

#include <cstdint>

/**
 * @brief Auto-repeat state for one held input: delay phase, then a fast interval.
 *
 * Owns only the timing and the identity of the repeating input (an opaque code plus a
 * modifier mask captured at press time); the owner decides what firing means. Used by the
 * game-controller input for held pad inputs and by the on-screen keyboard for held keys.
 * Deadlines are SDL_GetTicks64 times, so the event loop can wait on them with a timeout.
 */
class InputRepeater final {
public:
    /** Milliseconds a press stays held before the first repeat fires. */
    static constexpr uint64_t REPEAT_DELAY_MS = 400;

    /** Milliseconds between repeats once the first one fired. */
    static constexpr uint64_t REPEAT_INTERVAL_MS = 40;

private:
    /** True while an input is repeating. */
    bool m_armed;

    /** Opaque identity of the repeating input, owner-defined. */
    int32_t m_code;

    /** Modifier mask captured when the repeating input was pressed. */
    uint16_t m_modifiers;

    /** SDL_GetTicks64 time at which the armed repeat fires. */
    uint64_t m_deadline;

public:
    /** @brief Constructs a disarmed repeater. */
    explicit InputRepeater();

    /**
     * @brief Arms the delay phase for a newly pressed input.
     *
     * @param code Opaque identity of the input, used by disarmIf and returned by getCode.
     * @param modifiers The modifier mask held at press time.
     */
    void arm(int32_t code, uint16_t modifiers);

    /** @brief Disarms the repeat unconditionally. */
    void disarm();

    /**
     * @brief Disarms the repeat when the released input is the repeating one.
     *
     * @param code Opaque identity of the released input.
     */
    void disarmIf(int32_t code);

    /** @brief Re-arms at the fast interval after a fire, from the current time. */
    void rearm();

    /** @brief Tells whether an input is repeating, so the event loop waits with a timeout. */
    [[nodiscard]] bool isArmed() const;

    /** @brief Tells whether the armed repeat reached its deadline. */
    [[nodiscard]] bool isDue() const;

    /** @brief Gets the opaque identity of the repeating input. */
    [[nodiscard]] int32_t getCode() const;

    /** @brief Gets the modifier mask captured when the repeating input was pressed. */
    [[nodiscard]] uint16_t getModifiers() const;

    /** @brief Gets the SDL_GetTicks64 deadline of the armed repeat. */
    [[nodiscard]] uint64_t getDeadline() const;
};


#endif //INPUT_REPEATER_H
