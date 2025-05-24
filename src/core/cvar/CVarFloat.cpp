#include "CVarFloat.h"

#include <string>
#include <utf8.h>


CVarFloat::CVarFloat(const float value, const bool isReadOnly)
    : TypedCVar(value, isReadOnly) {}

std::optional<std::u16string> CVarFloat::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    try {
        const auto arg = utf8::utf16to8(args[0]);
        m_value = std::stof(arg);
    } catch (...) {
        return u"Unable to convert argument to float";
    }

    return std::nullopt;
}

std::u16string CVarFloat::getStringValue() const {
    return utf8::utf8to16(std::to_string(m_value));
}
