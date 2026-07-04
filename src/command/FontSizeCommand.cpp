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
#include "FontSizeCommand.h"

#include <ranges>
#include <utf8/cpp17.h>


const U16StringMap<FontSizeCommand::Size> FontSizeCommand::SIZE_MAP = {
    { u"+", Size::PLUS },
    { u"-", Size::MINUS}
};

void FontSizeCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (size direction)
        return;
    }

    for (const auto &item : std::views::keys(SIZE_MAP)) {
        if (item.starts_with(input)) {
            itemCallback(item);
        }
    }
}

std::optional<std::u16string> FontSizeCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.size() != 1) {
        return u"Expected 1 argument.";
    }

    const auto size = mapSize(args[0]);
    const auto font_size = payload.theme.getFontSize();

    // The first argument mapped to a size direction (+/-)
    switch (size) {
        case Size::PLUS:
            payload.theme.setFontSize(font_size + 1);
        break;
        case Size::MINUS:
            payload.theme.setFontSize(font_size - 1);
        break;
        case Size::UNKNOWN:
            // We don't know the direction, so assume we got a size instead.
            try {
                const auto utf16_pixel_size = utf8::utf16to8(args[0]);
                const auto pixel_size = std::stoi(utf16_pixel_size);
                payload.theme.setFontSize(pixel_size);
            } catch (...) {
                return u"Cannot convert arguments to size.";
            }
        break;
    }

    payload.wants_redraw = true;
    return std::nullopt;
}

FontSizeCommand::Size FontSizeCommand::mapSize(const std::u16string_view size) {
    if (const auto &mapped_size = SIZE_MAP.find(size); mapped_size != SIZE_MAP.end()) {
        return mapped_size->second;
    }

    return Size::UNKNOWN;
}
