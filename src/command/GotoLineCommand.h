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
#ifndef GOTO_LINE_COMMAND_H
#define GOTO_LINE_COMMAND_H

#include <optional>
#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command moving the editor cursor to the start of a given line.
 *
 * The target line is 1-based as typed by the user and clamped to the valid range of the buffer,
 * so out-of-range values snap to the first or last line rather than failing.
 */
class GotoLineCommand final : public Command<CursorContext> {
public:
    /** @brief Deleted copy constructor. */
    GotoLineCommand(const GotoLineCommand &) = delete;

    /** @brief Deleted copy assignment operator. */
    GotoLineCommand &operator=(const GotoLineCommand &) = delete;

    /** @brief Constructs a GotoLineCommand. */
    explicit GotoLineCommand() = default;

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
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Moves the cursor to the start of the requested line.
     *
     * Expects a single 1-based line number. The target is clamped to the buffer's line range and any
     * active selection is cleared, matching a plain cursor movement.
     *
     * @param payload The cursor context that will be modified by this command.
     * @param args Command arguments: the target 1-based line number.
     * @return An optional message describing an argument error, std::nullopt on success.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //GOTO_LINE_COMMAND_H
