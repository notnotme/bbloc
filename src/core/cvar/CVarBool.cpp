#include "../cvar/CVarBool.h"

#include <string>
#include <utf8.h>


CVarBool::CVarBool(const bool value, const bool isReadOnly)
    : TypedCVar(value, isReadOnly) {}

std::u16string CVarBool::getStringValue() const {
    return m_value ? u"true" : u"false";
}

void CVarBool::provideValueCompletion(const int32_t componentIndex, const AutoCompleteCallback &itemCallback) const {
    if (componentIndex != 0) {
        // A boolean has a single component
        return;
    }

    itemCallback(u"false");
    itemCallback(u"true");
}

std::optional<std::u16string> CVarBool::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Argument expected: <value>.";
    }

    try {
        m_value = args[0] == u"true";
    } catch (...) {
        return u"Unable to convert argument to boolean";
    }

    return std::nullopt;
}
