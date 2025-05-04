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
private:
    std::optional<std::u16string> setValueFromStrings(const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] std::u16string getStringValue() const override;

public:
    explicit CVarInt(int32_t value, bool isReadOnly = false);
};


#endif //CVAR_INT_H
