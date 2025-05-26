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
     * The vector of arguments must contain 4 items containing the red, green, blue, and alpha value.
     *
     * @param args The string arguments to parse into a Color value.
     * @return Nothing in case of success, an error message otherwise.
     */
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
};


#endif //CVAR_COLOR_H
