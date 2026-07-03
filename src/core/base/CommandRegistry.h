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
#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <memory>
#include <string_view>

#include "Command.h"


/**
 * @brief Interface for registering and organizing console commands.
 *
 * This class defines an abstract registry that stores and manages command instances.
 * Derived implementations are responsible for mapping string names to command objects
 * and check collision during runtime.
 *
 * @tparam TPayload The type of payload that commands operate on.
 */
template<class TPayload>
class CommandRegistry {
public:
    /** @brief Virtual destructor for inheritance. */
    virtual ~CommandRegistry() = default;

    /**
     * @brief Registers a command under the specified name.
     *
     * Adds a new command to the registry. Command names should be unique, and
     * case-insensitive comparisons (if used) should be handled in the implementation.
     *
     * @param name The name used to reference the command in the console.
     * @param command A shared pointer to the Command instance to register.
     */
    virtual void registerCommand(std::u16string_view name, std::shared_ptr<Command<TPayload>> command) = 0;
};


#endif //COMMAND_REGISTRY_H
