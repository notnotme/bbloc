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
        auto parsed_length = std::size_t(0);
        const auto value = std::stof(arg, &parsed_length);
        if (parsed_length != arg.length()) {
            // Reject trailing garbage such as "4x"
            return u"Unable to convert argument to float";
        }

        m_value = value;
    } catch (...) {
        return u"Unable to convert argument to float";
    }

    return std::nullopt;
}

std::u16string CVarFloat::getStringValue() const {
    return utf8::utf8to16(std::to_string(m_value));
}
