#include "PasteTextCommand.h"

#include <SDL_clipboard.h>
#include <utf8.h>


void PasteTextCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> PasteTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Ge thetext from the clipboard and convert it to UTF-16 encoding.
    char *sdl_clipboard_text = SDL_GetClipboardText();
    const auto clipboard_text = std::string(sdl_clipboard_text);
    SDL_free(sdl_clipboard_text);

    const auto utf16_clipboard_text = utf8::utf8to16(clipboard_text);
    if (utf16_clipboard_text.empty()) {
        // If there is no text in the clipboard show a message.
        return u"Clipboard is empty.";
    }

    if (payload.cursor.getSelectedRange()) {
        // If there is a selection, then we need to erase it.
        const auto &edit = payload.cursor.eraseSelection();
        payload.highlighter.edit(edit.value());
    }

    // Append the text at the cursor position
    const auto &edit = payload.cursor.insert(utf16_clipboard_text);
    payload.highlighter.edit(edit);

    // Pasting text automatically deactivates any selection.
    payload.cursor.activateSelection(false);

    // Redraw and follow the cursor.
    payload.wants_redraw = true;
    payload.follow_indicator = true;
    return std::nullopt;
}

bool PasteTextCommand::isRunnable(const CursorContext &payload) {
    // This command only pastes text in the editor view
    return payload.focus_target == FocusTarget::Editor;
}
