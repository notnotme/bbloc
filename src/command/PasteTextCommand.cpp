/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "PasteTextCommand.h"

#include <SDL_clipboard.h>
#include <utf8.h>


void PasteTextCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
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
    payload.search.resetMatches();

    // Redraw and follow the cursor.
    payload.wants_redraw = true;
    payload.scroll.follow_indicator = true;
    return std::nullopt;
}
