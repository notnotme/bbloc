#include "FontSizeCommand.h"

#include <ranges>
#include <utf8/cpp17.h>


const std::unordered_map<std::u16string, FontSizeCommand::Size> FontSizeCommand::SIZE_MAP = {
    { u"+", Size::PLUS },
    { u"-", Size::MINUS}
};

void FontSizeCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    for (const auto &item : std::views::keys(SIZE_MAP)) {
        itemCallback(item);
    }
}

std::optional<std::u16string> FontSizeCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Expected 1 argument.";
    }

    const auto size = mapSize(args[0]);
    const auto font_size = payload.theme.getFontSize();

    // The first argument mapped to a size direction (+/-)
    switch (size) {
        case Size::PLUS:
            payload.theme.setFontSize(font_size + 1);
        break;
        case Size::MINUS:
            payload.theme.setFontSize(font_size - 1);
        break;
        case Size::UNKNOWN:
            // We don't know the direction, so assume we got a size instead.
            try {
                const auto utf16_pixel_size = utf8::utf16to8(args[0]);
                const auto pixel_size = std::stoi(utf16_pixel_size);
                payload.theme.setFontSize(pixel_size);
            } catch (...) {
                return u"Cannot convert arguments to size.";
            }
        break;
    }

    payload.wants_redraw = true;
    return std::nullopt;
}

FontSizeCommand::Size FontSizeCommand::mapSize(const std::u16string_view size) {
    const auto size_str = std::u16string(size.begin(), size.end());
    if (const auto &mapped_size = SIZE_MAP.find(size_str); mapped_size != SIZE_MAP.end()) {
        return mapped_size->second;
    }

    return Size::UNKNOWN;
}
