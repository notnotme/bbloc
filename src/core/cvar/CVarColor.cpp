#include "../cvar/CVarColor.h"

#include <format>
#include <string>
#include <utf8.h>


CVarColor::CVarColor(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha, const bool isReadOnly)
    : TypedCVar({red, green, blue, alpha}, isReadOnly) {}

std::optional<std::u16string> CVarColor::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() < 3 || args.size() > 4) {
        return u"Argument expected: <red> <green> <blue> [alpha].";
    }

    try {
        const auto arg_r = std::stoi(utf8::utf16to8(args[0]));
        const auto arg_g = std::stoi(utf8::utf16to8(args[1]));
        const auto arg_b = std::stoi(utf8::utf16to8(args[2]));
        const auto arg_a = args.size() >= 4 ? std::stoi(utf8::utf16to8(args[3])) : 255;
        m_value.red = arg_r;
        m_value.green = arg_g;
        m_value.blue = arg_b;
        m_value.alpha = arg_a;
    } catch (...) {
        return u"Unable to convert arguments to color";
    }

    return std::nullopt;
}

std::u16string CVarColor::getStringValue() const {
    const auto arg_r = std::to_string(m_value.red);
    const auto arg_g = std::to_string(m_value.green);
    const auto arg_b = std::to_string(m_value.blue);
    const auto arg_a = std::to_string(m_value.alpha);
    return utf8::utf8to16(std::format("{} {} {} {}", arg_r, arg_g, arg_b, arg_a));
}
