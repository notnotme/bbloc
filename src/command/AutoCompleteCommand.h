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
#ifndef AUTO_COMPLETE_COMMAND_H
#define AUTO_COMPLETE_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command that triggers and manages auto-completion functionality in the text editor.
 *
 * This class implements the Command interface for handling auto-completion operations.
 * It interacts with the prompt state to provide contextual completion suggestions
 * based on the current cursor text and context.
 */
class AutoCompleteCommand final : public Command<CursorContext> {
private:
    /** @brief Represent a direction in the form of enum. */
    enum class Direction {
        UNKNOWN,    ///< Unknown direction.
        FORWARD,    ///< Direction goes forward.
        BACKWARD    ///< Direction goes backward.
    };

private:
    /** Lookup map to ease mapping Direction. */
    static const std::unordered_map<std::u16string, Direction> DIRECTION_MAP;

    /** Reference to the prompt state this command will use for auto-completion. */
    PromptState &m_prompt_state;

private:
    /**
     * @brief Map a string representation of a Direction to a Direction enum.
     *
     * Values can be "forward", "backward".
     *
     * @param direction The string representation of the direction.
     * @return The Direction enum corresponding to the string, or Unknown.
     */
    static Direction mapDirection(std::u16string_view direction);

public:
    /**
     * @brief Constructs an AutoCompleteCommand with the given prompt state.
     *
     * @param promptState Reference to the prompt state to use for auto-completion.
     */
    explicit AutoCompleteCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first argument with "forward" or "backward".
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the auto-completion command.
     *
     * Triggers the auto-completion mechanism based on the current cursor context.
     * 0 or 1 argument are allowed: "forward" and "backward". "forward" being the default, if not specified.
     *
     * @param payload The cursor context containing position and document information.
     * @param args Command arguments that may modify the auto-completion behavior.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //AUTO_COMPLETE_COMMAND_H
