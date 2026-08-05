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
#ifndef FONT_SIZE_COMMAND_H
#define FONT_SIZE_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/base/U16StringMap.h"


/**
 * @brief Command for adjusting the font size in the application.
 *
 * This class implements the Command interface for changing the display font size
 * of the application. It allows users to increase, decrease, or set the font size to
 * a specific value to improve readability based on their preferences.
 */
class FontSizeCommand final : public Command<CursorContext> {
private:
    /** @brief Represent a "size direction". */
    enum class Size {
        Unknown,
        Plus,
        Minus
    };

private:
    /** Lookup map to ease mapping font size. */
    static const U16StringMap<Size> SIZE_MAP;

    /**
     * Map a "size" argument into a Size. Argument "size" can be one of "+" or "-".
     * @param size The string representation of the size in the font unit.
     * @return The corresponding Size, or Unknown.
     */
    static Size mapSize(std::u16string_view size);

public:
    /** @brief Constructs a FontSizeCommand with default initialization. */
    explicit FontSizeCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first argument with "+" or "-".
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the font size adjustment.
     *
     * Changes the editor's font size based on the provided arguments. Which can be:
     * - "+" to increase the font size by one unit.
     * - "-" to decrease the font size by one unit.
     * - "x" which is an integer value to set the font size to a defined value directly.
     *
     * The font size may be clamped by the application if it does not fit certain condition.
     *
     * @param payload The cursor context.
     * @param args Command arguments specifying how to adjust the font size.
     * @return An optional message indicating the new font size or the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //FONT_SIZE_COMMAND_H
