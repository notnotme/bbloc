#include "SetHighLightCommand.h"

#include <utf8.h>


void SetHighLightCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
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
    const auto extension = std::string(".").append(utf8::utf16to8(args[0]));
    if (!HighLighter::isSupported(extension)) {
        return std::u16string(u"Unsupported highlight mode: ").append(args[0]);
    }

    payload.highlighter.setMode(extension);
    payload.wants_redraw = true;
    return std::nullopt;
}
