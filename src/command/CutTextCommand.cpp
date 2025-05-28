#include "CutTextCommand.h"

#include <SDL_clipboard.h>
#include <utf8.h>


void CutTextCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> CutTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    const auto &selection = payload.cursor.getSelectedText();
    if (!selection) {
        return u"Selection is empty.";
    }

    // Using this to store all the text at once and converting it to utf8 later in one shot, for SDL.
    auto to_clipboard_text = std::u16string();

    const auto &all_text = selection.value();
    const auto all_text_size = all_text.size();
    for (auto i = 0; i < all_text_size; ++i) {
        to_clipboard_text = to_clipboard_text.append(all_text[i]);
        if (i < all_text_size - 1) {
            // Because the selected text returns a vector, we need to append line ending.
            to_clipboard_text = to_clipboard_text.append(u"\n");
        }
    }

    if (const auto &edit = payload.cursor.eraseSelection()) {
        // If we had some text selected, then erase it.
        payload.highlighter.edit(edit.value());
        payload.cursor.activateSelection(false);

        // If we cut some text, a redrawing is needed.
        payload.wants_redraw = true;
    }

    // Need to convert the string to UTF-8 for SDL_SetClipboardText
    const auto utf8_clipboard_text = utf8::utf16to8(to_clipboard_text);
    SDL_SetClipboardText(utf8_clipboard_text.data());

    return std::nullopt;
}

bool CutTextCommand::isRunnable(const CursorContext &payload) {
    return payload.focus_target == FocusTarget::Editor;
}
