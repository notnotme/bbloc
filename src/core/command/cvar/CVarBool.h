#ifndef CVAR_BOOL_H
#define CVAR_BOOL_H

#include <string>
#include <string_view>
#include <optional>

#include "TypedCvar.h"


/**
 * @brief Boolean-based configuration variable.
 *
 * Allows storing and modifying a boolean value through the command system.
 */
class CVarBool final : public TypedCvar<bool> {
private:
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] std::u16string getStringValue() const override;

public:
    explicit CVarBool(bool value, bool isReadOnly = false);
};


#endif //CVAR_BOOL_H
