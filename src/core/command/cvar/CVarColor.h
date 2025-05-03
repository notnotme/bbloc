#ifndef CVAR_COLOR_H
#define CVAR_COLOR_H

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

#include "Color.h"
#include "TypedCvar.h"


/**
 * @brief Color-based configuration variable.
 *
 * Allows storing and modifying a color value through the command system.
 */
class CVarColor final : public TypedCvar<Color> {
private:
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] std::u16string getStringValue() const override;

public:
    /**
     * @brief Constructs a new color CVar with RGBA values.
     * @param red Red channel.
     * @param green Green channel.
     * @param blue Blue channel.
     * @param alpha Alpha channel.
     * @param isReadOnly Whether the CVar is immutable at runtime.
     */
    explicit CVarColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, bool isReadOnly = false);
};


#endif //CVAR_COLOR_H
