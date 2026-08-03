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


void PasteTextCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> PasteTextCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Ge thetext from the clipboard and convert it to UTF-16 encoding.
    char *sdl_clipboard_text = SDL_GetClipboardText();
    const auto clipboard_text = std::string(sdl_clipboard_text);
    SDL_free(sdl_clipboard_text);

    // The clipboard is not guaranteed to hold UTF-8: on X11, SDL falls back to XA_STRING (Latin-1).
    // Catch the base exception, a truncated trailing sequence raises not_enough_room, not invalid_utf8.
    // Never substitute U+FFFD here: this text goes into the document, silent corruption is worse than a refusal.
    auto utf16_clipboard_text = std::u16string();
    try {
        utf16_clipboard_text = utf8::utf8to16(clipboard_text);
    } catch (const utf8::exception &) {
        return u"Clipboard is not valid UTF-8.";
    }

    if (utf16_clipboard_text.empty()) {
        // If there is no text in the clipboard show a message.
        return u"Clipboard is empty.";
    }

    // Pasting over a selection replaces it, and deactivates it either way.
    payload.eraseSelectionIfAny();

    // Append the text at the cursor position
    const auto &edit = payload.cursor.insert(utf16_clipboard_text);
    payload.highlighter.edit(edit);

    // The pasted text moved the cursor: the next vertical move must aim at the column it landed
    // on, not at the one the last up/down move armed on a previous line.
    payload.stick.index = payload.cursor.getColumn();
    payload.search.resetMatches();

    // Redraw and follow the cursor.
    payload.wants_redraw = true;
    payload.scroll.follow_indicator = true;
    return std::nullopt;
}
