#ifndef CVAR_BOOL_H
#define CVAR_BOOL_H

#include <string>
#include <string_view>
#include <optional>

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
};


#endif //CVAR_BOOL_H
