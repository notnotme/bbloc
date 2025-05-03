#include "../cvar/CVarBool.h"

#include <string>
#include <utf8.h>


CVarBool::CVarBool(const bool value, const bool isReadOnly)
    : TypedCvar(value, isReadOnly) {}

std::optional<std::u16string> CVarBool::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    try {
        const auto arg = utf8::utf16to8(args[0]);
        m_value = arg == "true";
    } catch (...) {
        return u"Unable to convert argument to boolean";
    }

    return std::nullopt;
}

std::u16string CVarBool::getStringValue() const {
    return m_value ? u"true" : u"false";
}
