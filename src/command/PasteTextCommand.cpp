#include "PasteTextCommand.h"

#include <SDL_clipboard.h>
#include <utf8.h>

void PasteTextCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> PasteTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    char *sdl_clipboard_text = SDL_GetClipboardText();
    const auto clipboard_text = std::string(sdl_clipboard_text);
    const auto utf16_clipboard_text = utf8::utf8to16(clipboard_text);
    if (utf16_clipboard_text.empty()) {
        SDL_free(sdl_clipboard_text);
        return u"Clipboard is empty.";
    }

    if (payload.cursor.getSelectedRange()) {
        const auto &edit = payload.cursor.eraseSelection();
        payload.highlighter.edit(edit.value());
    }

    const auto &edit = payload.cursor.insert(utf16_clipboard_text);
    payload.highlighter.edit(edit);
    payload.follow_indicator = true;

    SDL_free(sdl_clipboard_text);
    payload.cursor.activateSelection(false);

    payload.wants_redraw = true;
    return std::nullopt;
}

bool PasteTextCommand::isRunnable(const CursorContext &payload) {
    return payload.focus_target == FocusTarget::Editor;
}
