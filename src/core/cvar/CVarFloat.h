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
#ifndef CVAR_FLOAT_H
#define CVAR_FLOAT_H

#include <string>
#include <string_view>
#include <optional>

#include "TypedCVar.h"


/**
 * @brief Float-based configuration variable.
 *
 * Allows storing and modifying a float value through the command system.
 */
class CVarFloat final : public TypedCVar<float> {
public:
    /**
     * @brief Constructs a CVarFloat with the given float value and read-only flag.
     *
     * @param value The initial float value of this CVar.
     * @param isReadOnly Whether this CVar is read-only (default: false).
     */
    explicit CVarFloat(float value, bool isReadOnly = false);

    /** @brief Converts the current float value to a string representation. */
    [[nodiscard]] std::u16string getStringValue() const override;

    /**
     * @brief Sets the value using string arguments.
     *
     * The vector must contain only one element which is the float representation of the value.
     *
     * @param args The vector must contain only one element that is the string representation of the float.
     * @return Nothing in case of success, an error message otherwise.
     */
    std::optional<std::u16string> setValueFromStrings(std::span<const std::u16string_view> args) override;
};


#endif //CVAR_FLOAT_H
