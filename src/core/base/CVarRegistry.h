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
    virtual void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) = 0;
};


#endif //CVAR_REGISTRY_H
