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
#ifndef CVAR_H
#define CVAR_H

#include <string>
#include <string_view>
#include <optional>
#include <span>

#include "AutoCompleteCallback.h"


/**
 * @brief Base class representing a configuration variable (CVar).
 *
 * CVars allow the user to view or modify runtime parameters.
 * Derived classes implement support for specific types (e.g., int, color) by inheriting TypedCVar.
 */
class CVar {
private:
    /** Indicates whether this CVar is read-only. */
    const bool m_is_read_only;

public:
    /** @brief Deleted copy constructor. */
    CVar(const CVar &) = delete;

    /** @brief Deleted copy assignment operator. */
    CVar &operator=(const CVar &) = delete;

    /** @brief Virtual destructor for polymorphic base class. */
    virtual ~CVar() = default;

    /**
     * @brief Constructs a CVar.
     *
     * @param isReadOnly Whether this CVar is read-only.
     */
    explicit CVar(bool isReadOnly);

    /** @return true if the CVar cannot be modified. */
    [[nodiscard]] bool isReadOnly() const;

    /** @return The current value as a UTF-16 string. */
    [[nodiscard]] virtual std::u16string getStringValue() const = 0;

    /**
     * @brief Sets the CVar value using string arguments (to be implemented by subclasses).
     *
     * This must not take into account the read-only status of the CVar.
     *
     * @param args The parsed arguments list.
     * @return An optional message string in return.
     */
    virtual std::optional<std::u16string> setValueFromStrings(std::span<const std::u16string_view> args) = 0;

    /**
     * @brief Provides completion suggestions for one component of the CVar value.
     *
     * The default implementation emits the componentIndex-th space-separated component
     * of getStringValue(), if it exists. Subclasses can override it to offer better candidates.
     *
     * @param componentIndex The index of the value component being completed.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    virtual void provideValueCompletion(int32_t componentIndex, const AutoCompleteCallback &itemCallback) const;
};

inline CVar::CVar(const bool isReadOnly)
    : m_is_read_only(isReadOnly) {}

inline bool CVar::isReadOnly() const {
    return m_is_read_only;
}

inline void CVar::provideValueCompletion(const int32_t componentIndex, const AutoCompleteCallback &itemCallback) const {
    const auto value = getStringValue();

    auto index = 0;
    auto component_index = 0;
    while (index < value.length()) {
        constexpr auto space_delimiter = u' ';

        // Skip blank spaces
        if (value[index] == space_delimiter) {
            ++index;
            continue;
        }

        const auto start = index;
        while (index < value.length() && value[index] != space_delimiter) {
            ++index;
        }

        if (component_index == componentIndex) {
            itemCallback(std::u16string_view(value).substr(start, index - start));
            return;
        }

        ++component_index;
    }
}


#endif //CVAR_H
