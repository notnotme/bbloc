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
#ifndef CVAR_REGISTRY_H
#define CVAR_REGISTRY_H

#include <memory>
#include <string_view>

#include "CVar.h"
#include "CVarCallback.h"


/**
 * @brief Interface for registering and handling configuration variables.
 *
 * Implementations of this class manage configuration variables (CVars)
 * and allow for dynamic updates through callback hooks.
 *
 * Derived implementations are responsible for mapping string names to command objects
 * and check collision during runtime.
 */
class CVarRegistry {
public:
    virtual ~CVarRegistry() = default;

    /**
     * @brief Registers a new configuration variable (CVar).
     *
     * Associates a named CVar with the registry and optionally sets a callback to be
     * triggered when the variable's value changes.
     *
     * @param name Variable name.
     * @param cvar Shared pointer to the CVar instance.
     * @param callback Optional callback invoked on changes.
     */
    virtual void registerCvar(std::u16string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) = 0;
};


#endif //CVAR_REGISTRY_H
