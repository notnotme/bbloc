#ifndef CVAR_FLOAT_H
#define CVAR_FLOAT_H

#include <string>
#include <string_view>
#include <optional>

#include "TypedCvar.h"


/**
 * @brief Float-based configuration variable.
 *
 * Allows storing and modifying a float value through the command system.
 */
class CVarFloat final : public TypedCvar<float> {
private:
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] std::u16string getStringValue() const override;

public:
    explicit CVarFloat(float value, bool isReadOnly = false);
};


#endif //CVAR_FLOAT_H
