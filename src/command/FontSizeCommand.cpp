#include "FontSizeCommand.h"

#include <utf8/cpp17.h>

void FontSizeCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) input;
    if (argumentIndex == 0) {
        itemCallback("+");
        itemCallback("-");
    }
}

std::optional<std::u16string> FontSizeCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Expected 1 argument.";
    }

    int32_t size = -1;
    int increase = false;
    if (args[0] == u"-") {
        increase = false;
    } else if (args[0] == u"+") {
        increase = true;
    } else {
        try {
            size = std::stoi(utf8::utf16to8(args[0]));
        } catch (...) {
            return u"Cannot convert arguments to size.";
        }
    }

    if (size == -1) {
        const auto font_size = payload.theme.getFontSize();
        if (increase) {
            payload.theme.setFontSize(font_size + 1);
        } else {
            payload.theme.setFontSize(font_size - 1);
        }
    } else {
        payload.theme.setFontSize(size);
    }

    payload.wants_redraw = true;
    return std::nullopt;
}
