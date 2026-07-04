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
#include "SetHighLightCommand.h"

#include <utf8.h>


void SetHighLightCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    // Ignore input
    (void) previousArgs;
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
        return std::u16string(u"Unsupported highlight mode: ").append(args[0]);
    }

    payload.highlighter.setMode(utf8_extension);
    payload.wants_redraw = true;
    return std::nullopt;
}
