#include "../cvar/CVarColor.h"

#include <format>
#include <stdexcept>
#include <string>
#include <utf8.h>


/** @brief Parses a color channel in [0, 255]; throws std::invalid_argument on any other input. */
static uint8_t parseChannel(const std::u16string_view arg) {
    const auto utf8_arg = utf8::utf16to8(arg);
    auto parsed_length = std::size_t(0);
    const auto value = std::stoi(utf8_arg, &parsed_length);
    if (parsed_length != utf8_arg.length() || value < 0 || value > 255) {
        throw std::invalid_argument("Color channel out of range");
    }

    return static_cast<uint8_t>(value);
}

CVarColor::CVarColor(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha, const bool isReadOnly)
    : TypedCVar({red, green, blue, alpha}, isReadOnly) {}

std::optional<std::u16string> CVarColor::setValueFromStrings(const std::vector<std::u16string_view> &args) {
    if (args.size() < 3 || args.size() > 4) {
        return u"Argument expected: <red> <green> <blue> [alpha].";
    }

    try {
        const auto arg_r = parseChannel(args[0]);
        const auto arg_g = parseChannel(args[1]);
        const auto arg_b = parseChannel(args[2]);
        const auto arg_a = args.size() >= 4 ? parseChannel(args[3]) : uint8_t(255);
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
    return utf8::utf8to16(std::format("{} {} {} {}",
        static_cast<uint32_t>(m_value.red), static_cast<uint32_t>(m_value.green),
        static_cast<uint32_t>(m_value.blue), static_cast<uint32_t>(m_value.alpha)));
}
