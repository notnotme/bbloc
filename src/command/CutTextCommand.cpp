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
#include "CutTextCommand.h"

#include "CopyTextCommand.h"


void CutTextCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> CutTextCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Copy first: on failure the selection is left untouched.
    if (const auto &error = CopyTextCommand::copySelectionToClipboard(payload)) {
        return error;
    }

    if (const auto &edit = payload.cursor.eraseSelection()) {
        // If we had some text selected, then erase it.
        payload.highlighter.edit(edit.value());
        payload.cursor.activateSelection(false);
        payload.search.resetMatches();

        // If we cut some text, a redrawing is needed.
        payload.wants_redraw = true;
    }

    return std::nullopt;
}
