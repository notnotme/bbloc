#include "../cvar/CVarInt.h"

#include <string>
#include <utf8.h>


CVarInt::CVarInt(const int32_t value, const bool isReadOnly)
    : TypedCVar(value, isReadOnly) {}

std::optional<std::u16string> CVarInt::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    try {
        const auto arg = utf8::utf16to8(args[0]);
        auto parsed_length = std::size_t(0);
        const auto value = std::stoi(arg, &parsed_length);
        if (parsed_length != arg.length()) {
            // Reject trailing garbage such as "4x"
            return u"Unable to convert argument to int";
        }

        m_value = value;
    } catch (...) {
        return u"Unable to convert argument to int";
    }

    return std::nullopt;
}

std::u16string CVarInt::getStringValue() const {
    return utf8::utf8to16(std::to_string(m_value));
}
