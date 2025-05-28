#include "SetHighLightCommand.h"

#include <utf8.h>


void SetHighLightCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    // Ignore input
    (void) input;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (mode)
        return;
    }

    HighLighter::getParserCompletions(itemCallback);
}

std::optional<std::u16string> SetHighLightCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Usage: set_hl_mode <mode>";
    }

    // Developers are lazy, so let's prepend a dot to make it work flawlessly
    const auto extension = std::u16string(u".").append(args[0]);
    const auto utf8_extension = utf8::utf16to8(extension);
    if (!HighLighter::isSupported(utf8_extension)) {
        return std::u16string(u"Unsupported highlight mode: ").append(extension);
    }

    payload.highlighter.setMode(utf8_extension);
    payload.wants_redraw = true;
    return std::nullopt;
}
