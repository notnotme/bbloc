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
#ifndef GLOBAL_REGISTRY_H
#define GLOBAL_REGISTRY_H

#include "CommandRegistry.h"
#include "CVarRegistry.h"


/**
 * @brief Combines command and configuration variable registries into a single interface.
 *
 * This class acts as a central registry, allowing systems to register both commands and CVars
 * through a unified interface. It inherits from both CommandRegistry (templated on TPayload)
 * and CVarRegistry.
 *
 * @tparam TPayload The payload type passed to registered commands.
 */
template<typename TPayload>
class GlobalRegistry : public CommandRegistry<TPayload>, public CVarRegistry {
public:
    /** @brief Virtual destructor for inheritance. */
    ~GlobalRegistry() override = default;
};


#endif //GLOBAL_REGISTRY_H
