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
#ifndef COMMAND_RUNNER_H
#define COMMAND_RUNNER_H

#include <span>
#include <string_view>

#include <SDL_keycode.h>


/**
 * @brief Abstract interface for executing user commands and providing auto-completion suggestions.
 *
 * This class handles parsing, dispatching, and completing console commands entered
 * by users in the prompt. Implementations should manage command execution flow and
 * intelligent suggestions for command names, arguments, and feedback.
 */
class CommandRunner {
public:
    virtual ~CommandRunner() = default;

    /**
     * @brief Executes the provided command input.
     *
     * @param input The full command input string, including arguments.
     * @param fromPrompt Indicates whether the input came from the interactive command prompt.
     * @return true if a command was successfully executed; false if unrecognized.
     */
    virtual bool runCommand(std::u16string_view input, bool fromPrompt) = 0;

    /**
     * @brief Looks up the key binding and runs the bound command, if any.
     *
     * The execution is timed by implementations (inf_command_time). Controller inputs,
     * encoded as pad pseudo-keycodes, dispatch through this path too.
     *
     * @param keycode The pressed key, or a pad pseudo-keycode.
     * @param modifiers The active key modifiers.
     * @return true when a bound command ran; false when no binding matched or the command was dropped.
     */
    virtual bool runBoundCommand(SDL_Keycode keycode, uint16_t modifiers) = 0;

    /**
     * @brief Clears a displayed prompt message back to the ready state.
     *
     * Input handlers call it when a new input action begins — the new action is what
     * invalidates the message the previous command left on screen. It must run before the
     * input is dispatched, otherwise it would erase the message that dispatch just produced.
     * Implementations do nothing unless a message is currently displayed.
     */
    virtual void dismissMessage() = 0;

    /**
     * @brief Provides auto-completion suggestions for command names.
     *
     * @param input The current (partial) user input string.
     * @param itemCallback Callback to receive possible command name completions.
     */
    virtual void getCommandCompletions(std::u16string_view input, const AutoCompleteCallback &itemCallback) = 0;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * @param command The name of the command being executed.
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The zero-based index of the argument currently being completed.
     * @param input The current (partial) user input for this argument.
     * @param itemCallback Callback to receive possible argument completions.
     */
    virtual void getArgumentsCompletions(std::u16string_view command, std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) = 0;
};


#endif //COMMAND_RUNNER_H
