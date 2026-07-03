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
#ifndef CVAR_BOOL_H
#define CVAR_BOOL_H

#include <string>
#include <string_view>
#include <optional>

#include "../base/AutoCompleteCallback.h"
#include "TypedCVar.h"


/**
 * @brief Boolean-based configuration variable.
 *
 * Allows storing and modifying a boolean value through the command system.
 */
class CVarBool final : public TypedCVar<bool> {
public:
    /**
     * @brief Constructs a CVarBool with the given bool value and read-only flag.
     *
     * @param value The initial boolean value for the configuration variable
     * @param isReadOnly If true, the variable cannot be modified after creation (default: false)
     */
    explicit CVarBool(bool value, bool isReadOnly = false);

    /** @return The string representation of the current boolean value */
    [[nodiscard]] std::u16string getStringValue() const override;

    /**
     * @brief Sets the value of the configuration variable from string arguments
     *
     * The vector must contain only one element which is the boolean representation of the value.
     *
     * @param args Vector of string arguments to parse into a boolean value.
     * @return An error message if the conversion fails, or std::nullopt if successful
     */
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Provides completion suggestions for the boolean value.
     *
     * Emits "false" and "true" for the first component; other components have no candidates.
     *
     * @param componentIndex The index of the value component being completed.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideValueCompletion(int32_t componentIndex, const AutoCompleteCallback &itemCallback) const override;
};


#endif //CVAR_BOOL_H
