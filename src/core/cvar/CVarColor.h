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
#ifndef CVAR_COLOR_H
#define CVAR_COLOR_H

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

#include "Color.h"
#include "TypedCVar.h"


/**
 * @brief Color-based configuration variable.
 *
 * Allows storing and modifying a Color value through the command system.
 */
class CVarColor final : public TypedCVar<Color> {
public:
    /**
     * @brief Constructs a CVarColor with the given Color value and read-only flag.
     *
     * @param red Red channel value.
     * @param green Green channel value.
     * @param blue Blue channel value.
     * @param alpha Alpha channel value.
     * @param isReadOnly Whether this CVar is read-only (default: false).
     */
    explicit CVarColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, bool isReadOnly = false);

    /** @brief Converts the current Color value to a string representation. */
    [[nodiscard]] std::u16string getStringValue() const override;

    /**
     * @brief Sets the Color value using string arguments.
     *
     * The vector of arguments must contain 3 items (red, green, blue), with an optional 4th alpha item defaulting to 255.
     *
     * @param args The string arguments to parse into a Color value.
     * @return Nothing in case of success, an error message otherwise.
     */
    std::optional<std::u16string> setValueFromStrings(std::span<const std::u16string_view> args) override;
};


#endif //CVAR_COLOR_H
