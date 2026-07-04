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
#ifndef COMMAND_H
#define COMMAND_H

#include <optional>
#include <span>
#include <string>

#include "AutoCompleteCallback.h"


/**
 * @brief Abstract base class for all console commands in the text editor.
 *
 * This templated class provides the interface for defining custom commands that
 * can be executed with a specific payload and optional arguments. It also supports
 * command-line auto-completion and runtime validation.
 *
 * @tparam TPayload The type of the payload passed to the command at execution.
 */
template <typename TPayload>
class Command {
public:
    /** @brief Deleted copy constructor. */
    Command(const Command &) = delete;

    /** @brief Deleted copy assignment operator. */
    Command &operator=(const Command &) = delete;

    /** @brief Constructs the Command with default values. */
    explicit Command() = default;

    /** @brief Virtual destructor for inheritance. */
    virtual ~Command() = default;

    /**
     * @brief Executes the command using the provided payload and arguments.
     *
     * @param payload The payload to use for running this command.
     * @param args The arguments to pass to this command.
     * @return An optional informative or error message.
     */
    [[nodiscard]] virtual std::optional<std::u16string> run(TPayload &payload, std::span<const std::u16string_view> args) = 0;

    /**
     * @brief Command-line completion function used to provide completion suggestions for command arguments.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    virtual void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const = 0;
};


#endif //COMMAND_H
