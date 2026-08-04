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
#ifndef QUIT_COMMAND_H
#define QUIT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/CursorContextManager.h"
#include "../core/base/Command.h"


/**
 * @brief Command for exiting the application.
 *
 * This class implements the Command interface for quitting/exiting the application.
 * It handles the graceful shutdown of the editor, performing cleanup operations before termination.
 *
 * When any open buffer holds unsaved changes, a single confirmation is asked before quitting.
 */
class QuitCommand final : public Command<CursorContext> {
private:
    /** Reference to the manager owning the open cursor contexts, scanned for unsaved changes. */
    CursorContextManager &m_context_manager;

public:
    /**
     * @brief Constructs a QuitCommand with a reference to the cursor context manager.
     *
     * @param contextManager Reference to the manager owning the open cursor contexts.
     */
    explicit QuitCommand(CursorContextManager &contextManager);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not auto-complete.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the quit operation.
     *
     * Initiates the application shutdown process. When any open buffer holds unsaved
     * changes, a confirmation is requested first; answering "y" re-runs the command
     * with the "-f" argument, which skips the check.
     *
     * @param payload The cursor context that carries the confirmation feedback, if needed.
     * @param args Command arguments; empty, or the single "-f" flag skipping the confirmation.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //QUIT_COMMAND_H
