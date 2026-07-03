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
#include "CopyTextCommand.h"

#include <SDL_clipboard.h>
#include <utf8.h>


void CopyTextCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> CopyTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    return copySelectionToClipboard(payload);
}

std::optional<std::u16string> CopyTextCommand::copySelectionToClipboard(CursorContext &payload) {
    const auto &selection = payload.cursor.getSelectedText();
    if (!selection) {
        return u"Selection is empty.";
    }

    // Because the selected text returns a vector, join the lines back with line endings.
    auto to_clipboard_text = std::u16string();
    auto is_first_line = true;
    for (const auto &line : selection.value()) {
        if (!is_first_line) {
            to_clipboard_text.append(u"\n");
        }
        to_clipboard_text.append(line);
        is_first_line = false;
    }

    // Need to convert the string to UTF-8 for SDL_SetClipboardText
    try {
        const auto utf8_clipboard_text = utf8::utf16to8(to_clipboard_text);
        SDL_SetClipboardText(utf8_clipboard_text.data());
    } catch (const utf8::exception &) {
        return u"Could not encode selection as UTF-8.";
    }

    return std::nullopt;
}
