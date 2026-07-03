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
#ifndef SET_HIGH_LIGHT_COMMAND_H
#define SET_HIGH_LIGHT_COMMAND_H


#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for setting highlighting options in the editor.
 *
 * This class implements the Command interface for controlling text highlighting
 * behavior in the editor. It allows users to toggle syntax highlighting.
 */
class SetHighLightCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a SetHighLightCommand with default initialization. */
    explicit SetHighLightCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for highlighting options.
     *
     * This command auto-completes argument 0 which is the highlight mode.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the highlighting command.
     *
     * Changes the highlighting behavior based on the provided arguments.
     * Expect 1 argument which is the parser name.
     * See HighLighter::getParserCompletions for the supported modes.
     *
     * @param payload The cursor context containing the editor state to be modified.
     * @param args Command arguments that specify the highlighting options to apply.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //SET_HIGH_LIGHT_COMMAND_H
