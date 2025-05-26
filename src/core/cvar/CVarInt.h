#ifndef CVAR_INT_H
#define CVAR_INT_H

#include <string>
#include <string_view>
#include <optional>

#include "TypedCVar.h"


/**
 * @brief Integer-based configuration variable.
 *
 * Allows storing and modifying an int32_t value through the command system.
 */
class CVarInt final : public TypedCVar<int32_t> {
public:
    /**
     * @brief Constructs a CVarInt with the given int32_t value and read-only flag.
     *
     * @param value The initial int32_t value of this CVar.
     * @param isReadOnly Whether this CVar is read-only (default: false).
     */
    explicit CVarInt(int32_t value, bool isReadOnly = false);

    /** @brief Converts the current integer value to a string representation. */
    [[nodiscard]] std::u16string getStringValue() const override;

    /**
     * @brief Sets the value using string arguments.
     *
     * The vector must contain only one element which is the integer representation of the value.
     *
     * @param args The vector must contain only one element that is the string representation of the int.
     * @return Nothing in case of success, an error message otherwise.
     */
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
};


#endif //CVAR_INT_H
