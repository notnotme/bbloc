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
#ifndef BUFFER_COMMAND_H
#define BUFFER_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/CursorContextManager.h"
#include "../core/base/Command.h"


/**
 * @brief Command for navigating between the open buffers.
 *
 * This class implements the Command interface for buffer management,
 * allowing users to cycle through the open buffers ("next" / "prev"),
 * close the active one ("close"), or switch directly to a buffer by name.
 */
class BufferCommand final : public Command<CursorContext> {
private:
    /** Reference to the manager owning the open cursor contexts. */
    CursorContextManager &m_context_manager;

    /** @brief Builds the "buffer <index>/<count>: <name>" status message for the active context. */
    [[nodiscard]] std::u16string statusMessage();

public:
    /**
     * @brief Constructs a BufferCommand with a reference to the cursor context manager.
     *
     * @param contextManager Reference to the manager owning the open cursor contexts.
     */
    explicit BufferCommand(CursorContextManager &contextManager);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first argument with the actions
     * "next", "prev" and "close", plus the names of the open buffers.
     * After "close", the second argument completes to the "-f" flag.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the buffer operation.
     *
     * Expects 1 argument: "next" / "prev" cycle through the open buffers (wrapping around),
     * "close" closes the active buffer (asking confirmation first when it holds unsaved
     * changes, unless the "-f" flag follows), and any other value switches directly to
     * the open buffer with that name.
     *
     * @param payload The cursor context that was active when the command was invoked.
     * @param args Command arguments specifying the buffer operation.
     * @return A message indicating the newly active buffer, or an error message.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //BUFFER_COMMAND_H
